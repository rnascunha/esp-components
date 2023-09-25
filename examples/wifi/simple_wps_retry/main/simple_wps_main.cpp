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

#include "sys/error.hpp"
#include "sys/sys.hpp"
#include "sys/event.hpp"
#include "sys/net.hpp"

#include "wifi/station.hpp"

#include "wifi/simple_wps_retry.hpp"

static constexpr
const char* TAG = "SIMPLE_WPS_STATION";

#define MAX_RETRY_ATTEMPTS CONFIG_ESP_MAXIMUM_RETRY

struct wifi_cb {
  static void connected(void*, void*) {
    ESP_LOGI("CB",  "Connected");
  }

  static void connecting(void*, void*) {
    ESP_LOGI("CB",  "Connecting");
  }

  static void fail(void*, void*) {
    ESP_LOGI("CB",  "FAIL");
  }
};

extern "C" void app_main() {
  auto err = sys::default_net_init();
  if (err) {
    ESP_LOGE(TAG, "Erro initializing chip [%d]", err.value());
    return;
  }

  /**
   * WiFi configuration/connection
   */
  auto* net_handler = wifi::station::initiate();
  if (net_handler == nullptr) {
    ESP_LOGE(TAG, "Configure WiFi error");
    return;
  }

  wifi::station::simple_wps_retry<wifi_cb> retry(MAX_RETRY_ATTEMPTS);
  err = retry.start();
  if (err) {
    ESP_LOGE(TAG, "Erro initializing WPS WiFi [%d]", err.value());
    return;
  }
  ESP_LOGI(TAG, "start wps...");

  retry.wait();

  if (retry.is_connected()) {
    auto config = *wifi::station::config();
    auto ip = sys::net::ip(net_handler).ip;
    ESP_LOGI(TAG, "Connected! SSID: %s, Password: %s, IP:" IPSTR,
                  (const char*)config.sta.ssid,
                  (const char*)config.sta.password,
                  IP2STR(&ip));
  } else if (retry.failed()) {
    ESP_LOGE(TAG, "Failed");
    return;
  } else {
    ESP_LOGW(TAG, "Unexpected event");
    return;
  }

  while (true) {
    sys::delay(sys::time::max);
  }
}