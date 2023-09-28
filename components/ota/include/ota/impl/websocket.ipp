/**
 * @file websocket.cpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_OTA_OTA_IMPL_WEBSOCKET_IPP__
#define COMPONENTS_OTA_OTA_IMPL_WEBSOCKET_IPP__


#include "inttypes.h"
#include <cassert>
#include <chrono>

#include "sys/task.hpp"
#include "sys/sys.hpp"

#include "esp_log.h"

#include "esp_ota_ops.h"
#include "esp_app_format.h"

#include "ota/packet.hpp"
#include "ota/websocket.hpp"

#define OTA_TASK_NAME         "ota_task"
#define OTA_TASK_STACK_SIZE   8192
#define OTA_TASK_PRIORITY     5

namespace ota {

static constexpr const char*
TAG = "OTA WS";

template<std::size_t Size>
static void abort_task(info<Size>& inf, esp_ota_handle_t handler = 0) noexcept {
  inf.client.clear();
  inf.task = nullptr;
  sys::task_delete();
  if (handler != 0)
    esp_ota_abort(handler);
}

template<std::size_t Size>
static void ota_task(void* arg) {
  ESP_LOGI(TAG, "OTA task initated");
  info<Size>& inf = *((info<Size>*)arg);

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  if (configured != running) {
      ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08" PRIx32 ", but running from offset 0x%08" PRIx32,
                configured->address, running->address);
      ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
  }
  ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08" PRIx32 ")",
            running->type, running->subtype, running->address);

  const esp_partition_t *update_partition = esp_ota_get_next_update_partition(nullptr);
  if (update_partition == nullptr) {
    ESP_LOGE(TAG, "Get partition error");
    send_error(inf.client, error_code::get_partiotion_error);
    abort_task(inf);
    return;
  }
  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%" PRIx32,
            update_partition->subtype, update_partition->address);

  int binary_file_length = 0;
  /*deal with all receive packet*/
  bool image_header_was_checked = false;
  esp_ota_handle_t update_handle = 0 ;
  while (true) {
    send_state(inf.client, binary_file_length, info<Size>::buffer_size - sizeof(state_request));
    sys::notify_wait();
  
    if (!inf.client.is_valid()) {
      ESP_LOGI(TAG, "Task aborted");
      abort_task(inf);
      return;
    }

    const state_request* state = (state_request*)inf.frame.payload;
    const std::uint32_t data_read = inf.frame.len - sizeof(state_request);
    const std::uint8_t* data = inf.frame.payload + sizeof(state_request);
    if (image_header_was_checked == false) {
      ESP_LOGI(TAG, "Checking image");

      esp_app_desc_t new_app_info;
      if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
        // check current version with downloading
        memcpy(&new_app_info, &data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
        ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

        esp_app_desc_t running_app_info;
        if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
          ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
        }

        const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
        esp_app_desc_t invalid_app_info;
        if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
          ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
        }

        // check current version with last invalid partition
        if (last_invalid_app != nullptr) {
          if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
            ESP_LOGW(TAG, "New version is the same as invalid version.");
            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
            send_error(inf.client, error_code::invalid_version);
            abort_task(inf);
          }
        }
// #ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
        if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
          ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
          send_error(inf.client, error_code::same_version);
          abort_task(inf);
        }
// #endif

        image_header_was_checked = true;

        sys::error err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
        if (err) {
          ESP_LOGE(TAG, "esp_ota_begin failed (%s)", err.message());
          abort_task(inf, update_handle);
        }
        ESP_LOGI(TAG, "esp_ota_begin succeeded");
      } else {
        ESP_LOGE(TAG, "received package is not fit len");
        abort_task(inf, update_handle);
      }
    }
    sys::error err = esp_ota_write(update_handle, (const void *)data, data_read);
    if (err) {
      ESP_LOGE(TAG, "Error writting ota data");
      abort_task(inf, update_handle);
    }
    binary_file_length += data_read;
    ESP_LOGD(TAG, "Written image length %d", binary_file_length);

    if (state->is_end) break;
  }
  send_state(binary_file_length, *inf.server);

  ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
  sys::error err = esp_ota_end(update_handle);
  if (err) {
    if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
      ESP_LOGE(TAG, "Image validation failed, image is corrupted");
    } else {
      ESP_LOGE(TAG, "esp_ota_end failed (%s)!", err.message());
    }
    abort_task(inf);
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", err.message());
    abort_task(inf);
  }
  ESP_LOGI(TAG, "OTA task finished");

  inf.task = nullptr;
  inf.client.clear();

  ESP_LOGI(TAG, "Prepare to restart system!");
  esp_restart();

  sys::task_delete();
}

template<std::size_t Size>
sys::task_handle
wesocket_task(info<Size>& info) noexcept {
  return sys::task_create(&ota_task<Size>, OTA_TASK_STACK_SIZE, OTA_TASK_PRIORITY, &info);
}

template<std::size_t Size>
void
ws_cb<Size>::on_close (int sock, void* server) noexcept {
  ESP_LOGI(TAG, "Disconnected %d", sock);

  if (sock == info.client.fd) {
    info.client.clear();
    if (info.task != nullptr) {
      ESP_LOGW(TAG, "Deleting OTA task");
      if (info.task != nullptr)
        vTaskDelete(info.task);
      info.task = nullptr;
    }
    send_abort(abort_reason::user_disconnect, *((http::server*)server));
  }
}

template<std::size_t Size>
sys::error
ws_cb<Size>::on_data(websocket::request req) noexcept {
  ESP_LOGI(TAG, "Data received");

  auto ret = req.receive(info.frame, info.buffer);
  if (ret) {
    ESP_LOGI(TAG, "Failed to receive data [%d/%s]", ret.value(), ret.message());
    return ret;
  }
  
  auto cmd = (command)info.frame.payload[0];
  switch(cmd) {
    case command::start:
      return start(req);
    case command::state:
      return state(req);
    case command::abort:
      return abort(req);
    default:
      ESP_LOGW(TAG, "Command not found %u", (std::uint8_t)cmd);
      break;
  }
  return sys::error{};
}

template<std::size_t Size>
sys::error
ws_cb<Size>::start(websocket::request req) noexcept {
  if (info.frame.len != sizeof(start_request)) {
    ESP_LOGW(TAG, "Packet size error");
    return send_error(websocket::client(req), error_code::packet_size_error);
  }
  ESP_LOGI(TAG, "Starting OTA task...");
  if (info.task != nullptr) {
    ESP_LOGW(TAG, "Task already running");
    return send_error(websocket::client(req), error_code::already_running);
  }

  start_request* d = (start_request*)info.frame.payload;

  info.client = websocket::client(req);
  info.server = (http::server*)req.context();
  info.task = wesocket_task(info);
  info.file_size = d->total_size;

  ESP_LOGW(TAG, "Task started [%" PRIu32 "]", info.file_size);

  return sys::error{};
}

template<std::size_t Size>
sys::error
ws_cb<Size>::state(websocket::request) noexcept {
  ESP_LOGI(TAG, "Data received %d", info.frame.len);
  if (info.task != nullptr)
    sys::notify(info.task);
  else
    ESP_LOGW(TAG, "Invalid task! Aborting");
  return sys::error{};
}

template<std::size_t Size>
sys::error
ws_cb<Size>::abort(websocket::request req) {
  ESP_LOGI(TAG, "Aborting OTA...");
  auto c = websocket::client(req);
  if (info.task == nullptr) {
    ESP_LOGW(TAG, "Task not running");
    return send_error(c, error_code::not_running);
  }
  if (c != info.client) {
    ESP_LOGW(TAG, "Wrong user");
    return send_error(c, error_code::wrong_user);
  }

  info.client.clear();
  sys::notify(info.task);
  ESP_LOGI(TAG, "Aborted");
  return send_abort(abort_reason::user_request, *((http::server*)req.context()));
}

}  // namespace ota


#endif // COMPONENTS_OTA_OTA_IMPL_WEBSOCKET_IPP__
