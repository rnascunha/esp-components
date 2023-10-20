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

static abort_reason
check_update_image(const std::uint8_t* data,
                   bool check_last_invalid,
                   bool check_same_version) noexcept {
  if (!check_last_invalid && !check_same_version)
    return abort_reason::no_abort;
  
  esp_app_desc_t new_app_info;
  memcpy(&new_app_info, &data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
  ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

  if (check_last_invalid) {
    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
    esp_app_desc_t invalid_app_info;
    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
      ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
    }

    // check current version with last invalid partition
    if (last_invalid_app != nullptr && 
        memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
      ESP_LOGW(TAG, "New version is the same as invalid version.");
      ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
      ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
      return abort_reason::invalid_version;
    }
  }

  if (check_same_version) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    esp_ota_get_partition_description(running, &running_app_info);
    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
      ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
      return abort_reason::same_version;
    }
  }
  return abort_reason::no_abort;
}

template<std::size_t Size>
static void ota_task(void* arg) {
  ESP_LOGI(TAG, "OTA task initated");

  info<Size>& inf = *((info<Size>*)arg);
  esp_ota_handle_t update_handle = 0;
  
  auto abort_task = [&inf, &update_handle](abort_reason reason = abort_reason::no_abort){
    if (reason != abort_reason::no_abort)
      send_abort(inf.client, reason);
    inf.client.clear();
    inf.task = nullptr;
    if (update_handle != 0)
      esp_ota_abort(update_handle);
    sys::task_delete();
  };

  const esp_partition_t *update_partition = esp_ota_get_next_update_partition(nullptr);
  if (update_partition == nullptr) {
    ESP_LOGE(TAG, "Get partition error");
    abort_task(abort_reason::get_partiotion_error);
  }
  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%" PRIx32,
            update_partition->subtype, update_partition->address);

  int binary_file_length = 0;
  bool image_header_was_checked = false;
  auto timeout = inf.timeout == 0 ?
                    sys::time::max :
                    sys::time::to_ticks(std::chrono::milliseconds(inf.timeout));
  while (true) {
    send_state(inf.client, binary_file_length, info<Size>::buffer_size - sizeof(state_request));
    if (sys::notify_wait(timeout) == 0) {
      ESP_LOGW(TAG, "Timeout! Aborting");
      abort_task(abort_reason::timeout);
    }
  
    if (!inf.client.is_valid()) {
      ESP_LOGI(TAG, "Task aborted");
      abort_task();
    }

    const state_request* state = (state_request*)inf.frame.payload;
    const std::uint32_t data_read = inf.frame.len - sizeof(state_request);
    const std::uint8_t* data = inf.frame.payload + sizeof(state_request);
    if (image_header_was_checked == false) {
      ESP_LOGI(TAG, "Checking image");

      auto reason = check_update_image(data, inf.check_last_invalid, inf.check_same_version);
      if (reason != abort_reason::no_abort) {
        ESP_LOGE(TAG, "Image check error [%u]", (std::uint8_t)reason);
        abort_task(reason);
      }
      image_header_was_checked = true;

      sys::error err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
      if (err) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", err.message());
        abort_task(abort_reason::ota_begin_error);
      }
      ESP_LOGI(TAG, "esp_ota_begin succeeded");
    }
    sys::error err = esp_ota_write(update_handle, (const void *)data, data_read);
    if (err) {
      ESP_LOGE(TAG, "Error writting ota data");
      abort_task(abort_reason::ota_write_error);
    }
    binary_file_length += data_read;
    ESP_LOGD(TAG, "Written image length %d", binary_file_length);

    if (state->is_end) break;
  }
  send_state(inf.client, binary_file_length, 0);

  ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
  sys::error err = esp_ota_end(update_handle);
  if (err) {
    ESP_LOGE(TAG, "esp_ota_end failed (%s)!", err.message());
    abort_task(abort_reason::ota_end_error);
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", err.message());
    abort_task(abort_reason::set_partition_error);
  }
  ESP_LOGI(TAG, "OTA task finished");

  inf.task = nullptr;
  inf.client.clear();

  if (inf.reset) {
    ESP_LOGI(TAG, "Prepare to restart system!");
    sys::reboot();
  }

  sys::task_delete();
}

template<std::size_t Size>
static sys::task_handle
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
      if (info.task != nullptr)
        sys::notify(info.task);
      info.task = nullptr;
    }    
  }
}

template<std::size_t Size>
sys::error
ws_cb<Size>::on_data(websocket::request req) noexcept {
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
    case command::action:
      return action(req);
    default:
      ESP_LOGW(TAG, "Command not found %u", (std::uint8_t)cmd);
      return send_error(websocket::client(req), error_code::no_command_found);
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
  info.reset = d->reset;
  info.check_last_invalid = d->check_last_invalid;
  info.check_same_version = d->check_same_version;
  info.timeout = d->timeout;

  ESP_LOGI(TAG, "fsize: %" PRIu32 ", reset:%u, invalid:%u, same:%u, timeout:%" PRIu16,
                info.file_size,
                info.reset,
                info.check_last_invalid,
                info.check_same_version,
                info.timeout);
  ESP_LOGI(TAG, "Task started [%" PRIu32 "]", info.file_size);

  return sys::error{};
}

template<std::size_t Size>
sys::error
ws_cb<Size>::state(websocket::request) noexcept {
  if (info.task != nullptr)
    sys::notify(info.task);
  else
    ESP_LOGW(TAG, "Invalid task! Aborting");
  return sys::error{};
}

template<std::size_t Size>
sys::error
ws_cb<Size>::abort(websocket::request req) noexcept {
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

  auto ret = send_abort(info.client, abort_reason::user_request);
  info.client.clear();
  sys::notify(info.task);
  ESP_LOGI(TAG, "Aborted");
  return ret;
}

template<std::size_t Size>
sys::error
ws_cb<Size>::action(websocket::request req) noexcept {
  auto c = websocket::client(req);
  if (info.frame.len != sizeof(action_request)) {
    ESP_LOGW(TAG, "Packet size error");
    return send_error(c, error_code::packet_size_error);
  }

  if (info.task != nullptr) {
    ESP_LOGW(TAG, "No action! Uploading!");
    return send_error(c, error_code::action_wrong_time);
  }

  ota::action act = (ota::action)info.frame.payload[1];
  switch(act) {
    case action::reset:
      ESP_LOGI(TAG, "[ACTION] reset [%u]", (std::uint8_t)act);
      send_action(c, action::reset, 0);
      sys::reboot();
      break;
    case action::validate_image: {
        ESP_LOGI(TAG, "[ACTION] validate image [%u]", (std::uint8_t)act);
        auto err = esp_ota_mark_app_valid_cancel_rollback();
        send_action(c, action::validate_image, err);
      }
      break;
    case action::invalidate_image: {
        ESP_LOGI(TAG, "[ACTION] invalidate image [%u]", (std::uint8_t)act);
        auto err = esp_ota_mark_app_invalid_rollback_and_reboot();
        send_action(c, action::invalidate_image, err);
      }
      break;
    default:
      ESP_LOGW(TAG, "Action not found %u", (std::uint8_t)act);
      return send_error(websocket::client(req), error_code::no_action_found);
  }
  return sys::error{};
}

}  // namespace ota


#endif // COMPONENTS_OTA_OTA_IMPL_WEBSOCKET_IPP__
