/**
 * @file main.cpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <chrono>

#include "esp_log.h"

#include "sys/error.hpp"
#include "sys/sys.hpp"
#include "sys/time.hpp"
#include "sys/net.hpp"
#include "sys/task.hpp"

#include "wifi/common.hpp"
#include "wifi/station.hpp"
#include "wifi/simple_wifi_retry.hpp"

#include "websocket/server.hpp"

#include "ota/websocket.hpp"

#include "wifi_args.hpp"

static constexpr const char* TAG = "WS OTA MAIN";

#define BUFFER_SIZE (CONFIG_OTA_BUFFER_SIZE + sizeof(ota::state_request))

extern "C" void app_main() {
  auto err = sys::default_net_init();
  if (err) {
    ESP_LOGE(TAG, "Erro initializing chip [%d/%s]", err.value(), err.message());
    return;
  }

  /**
   * WiFi configuration/connection
   */
  wifi::config config = wifi::station::build_config(EXAMPLE_ESP_WIFI_SSID,
                                                    EXAMPLE_ESP_WIFI_PASS)
                          .sae_h2e_identifier(EXAMPLE_H2E_IDENTIFIER)
                          .authmode(ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD)
                          .sae_pwe_h2e(ESP_WIFI_SAE_MODE);

  auto* net_handler = wifi::station::initiate(config);
  if (net_handler == nullptr) {
    ESP_LOGE(TAG, "Configure WiFi error");
    return;
  }

  wifi::station::simple_wifi_retry retry{EXAMPLE_ESP_MAXIMUM_RETRY};
  http::server_connect_cb([](auto& server) {
    server.register_uri(
      websocket::uri<ota::ws_cb<BUFFER_SIZE>>{
        .uri            = "/ota",
        .user_ctx       = &server,
        .control_frames = false,
        .supported_subprotocol = nullptr
      }()
    );
  });
  err = wifi::start();
  if (err) {
    ESP_LOGE(TAG, "Connect WiFi error [%d/%s]", err.value(), err.message());
    return;
  }

  ESP_LOGI(TAG, "WiFi connecting to SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);

  retry.wait();

  if (retry.is_connected()) {
    auto ip = sys::net::ip(net_handler).ip;
    ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&ip));
  } else if (retry.failed()) {
    ESP_LOGW(TAG, "Failed");
    return;
  } else {
    ESP_LOGW(TAG, "Unexpected event");
    return;
  }

  while (true) {
    using namespace std::chrono_literals;
    sys::delay(sys::time::max);
  }
}