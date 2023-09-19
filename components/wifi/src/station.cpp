/**
 * @file station.cpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-04-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <cstring>
#include <string_view>

#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wps.h"

#include "sys/error.hpp"
#include "wifi/station.hpp"

namespace wifi {
namespace station {

esp_netif_t* initiate() noexcept {
  auto net = esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  sys::error err = esp_wifi_init(&cfg);
  if (err) {
    esp_netif_destroy(net);
    return nullptr;
  }
  err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (err) {
    esp_netif_destroy(net);
    return nullptr;
  }
  
  return net;
}

esp_netif_t* initiate(wifi::config& wifi_config) noexcept {
  auto net = initiate();
  if (net == nullptr)
    return nullptr;
  
  auto err = config(wifi_config);
  if (err) {
    esp_netif_destroy(net);
    return nullptr;
  }
  
  return net;
}

sys::error config(wifi::config& cfg) noexcept {
  return esp_wifi_set_config(WIFI_IF_STA, &cfg);
}

std::optional<wifi::config> config() noexcept {
  wifi::config cfg;
  auto err = esp_wifi_get_config(WIFI_IF_STA, &cfg);
  if (err != ESP_OK) return std::nullopt;
  return cfg;
}

sys::error wps_start() noexcept {
  return esp_wifi_wps_start(0 /* value ignored by API */);
}

sys::error wps_enable(const esp_wps_config_t& cfg) noexcept {
  return esp_wifi_wps_enable(&cfg);
}

sys::error wps_enable(wps_type type) noexcept {
  return wps_enable(WPS_CONFIG_INIT_DEFAULT(type));
}

sys::error wps_disable() noexcept {
  return esp_wifi_wps_disable();
}

}   // namespace station
}   // namespace wifi
