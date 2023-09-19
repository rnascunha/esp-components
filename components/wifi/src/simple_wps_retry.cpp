/**
 * @file simple_wps_retry.cpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-04-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <chrono>

#include "sys/task.hpp"

#include "sys/time.hpp"
#include "sys/event.hpp"

#include "wifi/common.hpp"
#include "wifi/station.hpp"
#include "wifi/simple_wps_retry.hpp"

namespace wifi {
namespace station {

simple_wps_retry::simple_wps_retry(
  int max_retry /* = std::numeric_limits<int>::max() */) noexcept
  : max_retry_{max_retry} {
  register_handler(*this);
}

simple_wps_retry::simple_wps_retry(not_register,
                                     int max_retry /* = std::numeric_limits<int>::max() */) noexcept
 : max_retry_{max_retry} {}

sys::error
simple_wps_retry::start() noexcept {
    auto err = wifi::start();
    if (err) return err;
    err = wifi::station::wps_enable(config_);
    if (err) return err;
    return wifi::station::wps_start();
}

void
simple_wps_retry::handler(void* arg,
            esp_event_base_t event_base,
            std::int32_t event_id,
            void* event_data) noexcept {
  simple_wps_retry* self = (simple_wps_retry*)arg;
  if (event_base == WIFI_EVENT)
    self->wifi_handler(arg, event_id, event_data);
  else if (event_base == IP_EVENT)
    self->ip_handler(arg, event_id, event_data);
}

void simple_wps_retry::reset() noexcept {
  retry_ = 0;
  event_.clear(connected | fail);
}

void 
simple_wps_retry::wifi_handler(void* arg,
                               int32_t event_id,
                               void* event_data) noexcept {
  switch (event_id) {
    case WIFI_EVENT_STA_DISCONNECTED:
      if (retry_ < max_retry_) {
        wifi::connect();
        ++retry_;
      } else if (++ap_idx_ < ap_creds_num_) {
        wifi::station::config(wps_ap_creds_[ap_idx_]);
        wifi::connect();
        retry_ = 0;
      } else
        event_.set(fail);

      break;
    case WIFI_EVENT_STA_WPS_ER_SUCCESS:
      {
        wifi_event_sta_wps_er_success_t *evt =
            (wifi_event_sta_wps_er_success_t *)event_data;

        if (evt) {
          ap_creds_num_ = evt->ap_cred_cnt;
          for (int i = 0; i < ap_creds_num_; i++) {
            memcpy(wps_ap_creds_[i].sta.ssid, evt->ap_cred[i].ssid,
                    sizeof(evt->ap_cred[i].ssid));
            memcpy(wps_ap_creds_[i].sta.password, evt->ap_cred[i].passphrase,
                    sizeof(evt->ap_cred[i].passphrase));
          }
          ap_idx_ = 0;
          wifi::station::config(wps_ap_creds_[ap_idx_]);
        }
        /*
         * If only one AP credential is received from WPS, there will be no event data and
         * esp_wifi_set_config() is already called by WPS modules for backward compatibility
         * with legacy apps. So directly attempt connection here.
         */
        wifi::station::wps_disable();
        wifi::connect();
      }
      break;
    case WIFI_EVENT_STA_WPS_ER_FAILED:
    case WIFI_EVENT_STA_WPS_ER_TIMEOUT:
      wifi::station::wps_disable();
      wifi::station::wps_enable(config_);
      wifi::station::wps_start();
      break;
    default:
      break;
  }
}

void
simple_wps_retry::ip_handler(void* arg,
                              std::int32_t event_id,
                              void* event_data) noexcept {
  retry_ = 0;
  event_.set(connected);
}

bool
simple_wps_retry::is_connected() const noexcept {
  return event_.is_set(connected);
}

bool
simple_wps_retry::failed() const noexcept {
  return event_.is_set(fail);
}

void register_handler(simple_wps_retry& instance) noexcept {
  wifi::register_handler(ESP_EVENT_ANY_ID, &simple_wps_retry::handler, &instance);
  sys::event::register_handler(IP_EVENT, IP_EVENT_STA_GOT_IP, &simple_wps_retry::handler, &instance);
}

}  // namespace station 
}  // namespace wifi


