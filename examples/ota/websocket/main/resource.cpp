#include "esp_log.h"

#include "websocket/server.hpp"

#include "ota/websocket.hpp"

#include "ota/packet.hpp"

static constexpr const
char* TAG = "OTA_WS_MAIN";

struct ws_cb {
  inline static ota::info info{};

  static sys::error on_open (websocket::request req) {
    ESP_LOGI(TAG, "Handshake done, the new connection was opened %d", req.socket());
    return ESP_OK;
  }

  static void on_close (int sock, void* server) {
    ESP_LOGI(TAG, "Disconnected %d", sock);

    if (sock == info.client.fd) {
      info.client.clear();
      if (info.task != nullptr) {
        ESP_LOGW(TAG, "Deleting OTA task");
        if (info.task != nullptr)
          vTaskDelete(info.task);
        info.task = nullptr;
      }
      ota::send_abort(ota::abort_reason::user_disconnect, *((http::server*)server));
    }
  }

  static sys::error on_data(websocket::request req) {
    ESP_LOGI(TAG, "Data received");

    websocket::data data;
    auto ret = req.receive(data);
    if (ret) {
      ESP_LOGI(TAG, "Failed to receive data [%d/%s]", ret.value(), ret.message());
      return ret;
    }
    
    auto cmd = (ota::command)data.packet.payload[0];
    switch(cmd) {
      case ota::command::start:
        return start(req, data);
        break;
      case ota::command::abort:
          return abort(req);
        break;
      default:
        ESP_LOGW(TAG, "Command not found %u", (std::uint8_t)cmd);
        break;
    }
    return sys::error{};
  }

  static sys::error start(websocket::request req, websocket::data& data) {
    ESP_LOGI(TAG, "Starting OTA task...");
    if (info.task != nullptr) {
      ESP_LOGW(TAG, "Task already running");
      return ota::send_error(websocket::client(req), ota::error_code::already_running);
    }

    info.client = websocket::client(req);
    info.server = (http::server*)req.context();
    info.task = ota::wesocket_task(info);
    ESP_LOGW(TAG, "Task started");
    return sys::error{};
  }

  static sys::error abort(websocket::request req) {
    ESP_LOGI(TAG, "Aborting OTA...");
    auto c = websocket::client(req);
    if (info.task == nullptr) {
      ESP_LOGW(TAG, "Task not running");
      return ota::send_error(c, ota::error_code::not_running);
    }
    if (c != info.client) {
      ESP_LOGW(TAG, "Wrong user");
      return ota::send_error(c, ota::error_code::wrong_user);
    }

    ESP_LOGI(TAG, "Aborted");
    return ota::send_abort(ota::abort_reason::user_request, *((http::server*)req.context()));
  }
};