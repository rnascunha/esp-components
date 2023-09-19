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

#include "esp_wifi.h"
#include "esp_wps.h"

#include "sys/error.hpp"
#include "sys/sys.hpp"
#include "sys/event.hpp"
#include "wifi/station.hpp"

static constexpr
const char* TAG = "WPS_STATION";

#define WPS_MODE WPS_TYPE_PBC
#define MAX_RETRY_ATTEMPTS CONFIG_ESP_MAXIMUM_RETRY

static esp_wps_config_t config = WPS_CONFIG_INIT_DEFAULT(WPS_MODE);
static wifi_config_t wps_ap_creds[MAX_WPS_AP_CRED];
static int s_ap_creds_num = 0;
static int s_retry_num = 0;

static void 
wifi_event_handler(void* arg, esp_event_base_t event_base,
                  int32_t event_id, void* event_data)
{
  static int ap_idx = 1;

  switch (event_id) {
    case WIFI_EVENT_STA_START:
      ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
      if (s_retry_num < MAX_RETRY_ATTEMPTS) {
        wifi::connect();
        s_retry_num++;
      } else if (ap_idx < s_ap_creds_num) {
        /* Try the next AP credential if first one fails */

        if (ap_idx < s_ap_creds_num) {
          ESP_LOGI(TAG, "Connecting to SSID: %s, Passphrase: %s",
                    wps_ap_creds[ap_idx].sta.ssid, wps_ap_creds[ap_idx].sta.password);
          ESP_ERROR_CHECK(wifi::station::config(wps_ap_creds[ap_idx++]));
          wifi::connect();
        }
        s_retry_num = 0;
      } else {
        ESP_LOGI(TAG, "Failed to connect!");
      }

      break;
    case WIFI_EVENT_STA_WPS_ER_SUCCESS:
      ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_SUCCESS");
      {
        wifi_event_sta_wps_er_success_t *evt =
            (wifi_event_sta_wps_er_success_t *)event_data;

        if (evt) {
          s_ap_creds_num = evt->ap_cred_cnt;
          for (int i = 0; i < s_ap_creds_num; i++) {
            memcpy(wps_ap_creds[i].sta.ssid, evt->ap_cred[i].ssid,
                    sizeof(evt->ap_cred[i].ssid));
            memcpy(wps_ap_creds[i].sta.password, evt->ap_cred[i].passphrase,
                    sizeof(evt->ap_cred[i].passphrase));
          }
          /* If multiple AP credentials are received from WPS, connect with first one */
          ESP_LOGI(TAG, "Connecting to SSID: %s, Passphrase: %s",
                    wps_ap_creds[0].sta.ssid, wps_ap_creds[0].sta.password);
          ESP_ERROR_CHECK(wifi::station::config(wps_ap_creds[ap_idx++]));
        }
        /*
          * If only one AP credential is received from WPS, there will be no event data and
          * esp_wifi_set_config() is already called by WPS modules for backward compatibility
          * with legacy apps. So directly attempt connection here.
          */
        ESP_ERROR_CHECK(wifi::station::wps_disable());
        wifi::connect();
      }
      break;
    case WIFI_EVENT_STA_WPS_ER_FAILED:
      ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_FAILED");
      ESP_ERROR_CHECK(wifi::station::wps_disable());
      ESP_ERROR_CHECK(wifi::station::wps_enable(config));
      ESP_ERROR_CHECK(wifi::station::wps_start());
      break;
    case WIFI_EVENT_STA_WPS_ER_TIMEOUT:
      ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_TIMEOUT");
      ESP_ERROR_CHECK(wifi::station::wps_disable());
      ESP_ERROR_CHECK(wifi::station::wps_enable(config));
      ESP_ERROR_CHECK(wifi::station::wps_start());
      break;
    default:
      break;
  }
}

static void
got_ip_event_handler(void* arg, esp_event_base_t event_base,
                     int32_t event_id, void* event_data) noexcept {
  ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
  ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
}

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

  wifi::register_handler(ESP_EVENT_ANY_ID, &wifi_event_handler);
  sys::event::register_handler(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_event_handler);

  err = wifi::start();
  if (err) {
    ESP_LOGE(TAG, "Erro initializing wifi [%d]", err.value());
    return;
  }

  ESP_LOGI(TAG, "start wps...");

  ESP_ERROR_CHECK(wifi::station::wps_enable(config));
  ESP_ERROR_CHECK(wifi::station::wps_start());
}