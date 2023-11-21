/**
 * @file simple_wifi_retry.cpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-04-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <chrono>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"

#include "sys/time.hpp"
#include "sys/event.hpp"

#include "wifi/common.hpp"
#include "wifi/simple_wifi_retry.hpp"

#include "wifi/detail/type_traits.hpp"

namespace wifi {
namespace station {

template<typename Callbacks>
simple_wifi_retry<Callbacks>::simple_wifi_retry(
  int max_retry /* = std::numeric_limits<int>::max() */) noexcept
  : max_retry_{max_retry} {
  register_handler(*this);
}

template<typename Callbacks>
simple_wifi_retry<Callbacks>::simple_wifi_retry(wifi::not_register,
                                     int max_retry /* = std::numeric_limits<int>::max() */) noexcept
 : max_retry_{max_retry} {}

template<typename Callbacks>
simple_wifi_retry<Callbacks>::~simple_wifi_retry() noexcept {
  sys::event::unregister_handler(WIFI_EVENT, ESP_EVENT_ANY_ID, &simple_wifi_retry<Callbacks>::handler);
  sys::event::unregister_handler(IP_EVENT, IP_EVENT_STA_GOT_IP, &simple_wifi_retry<Callbacks>::handler);
}

template<typename Callbacks>
EventBits_t 
simple_wifi_retry<Callbacks>::wait() noexcept {
  return event_.wait(connected | fail,
                     sys::time::max,
                     pdFALSE,
                     pdFALSE);
}

template<typename Callbacks>
EventBits_t 
simple_wifi_retry<Callbacks>::wait(sys::time::tick_time auto wait_time) noexcept {
  return event_.wait(connected | fail,
                     to_ticks(wait_time),
                     pdFALSE,
                     pdFALSE);
}

template<typename Callbacks>
void
simple_wifi_retry<Callbacks>::handler(void* arg,
            esp_event_base_t event_base,
            std::int32_t event_id,
            void* event_data) noexcept {
  simple_wifi_retry* self = (simple_wifi_retry*)arg;
  if (event_base == WIFI_EVENT)
    self->wifi_handler(arg, event_id, event_data);
  else if (event_base == IP_EVENT)
    self->ip_handler(arg, event_id, event_data);
}

template<typename Callbacks>
void
simple_wifi_retry<Callbacks>::reset() noexcept {
  retry_ = 0;
  event_.clear(connected | fail);
}

template<typename Callbacks>
void
simple_wifi_retry<Callbacks>::wifi_handler(void* arg,
                                std::int32_t event_id,
                                void* event_data) noexcept {
  switch(event_id) {
    case WIFI_EVENT_STA_START:
      esp_wifi_connect();
      if constexpr (wifi::detail::has_connecting_v<Callbacks>)
        Callbacks::connecting(arg, event_data);
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      if constexpr (wifi::detail::has_disconnected_v<Callbacks>)
        Callbacks::disconnected(arg, event_data);
      if (retry_++ < max_retry_) {
        esp_wifi_connect();
        if constexpr (wifi::detail::has_connecting_v<Callbacks>)
          Callbacks::connecting(arg, event_data);
      } else {
        event_.set(fail);
        if constexpr (wifi::detail::has_fail_v<Callbacks>)
          Callbacks::fail(arg, event_data);
      }
      break;
      case WIFI_EVENT_STA_CONNECTED:
        if constexpr (wifi::detail::has_station_connected_v<Callbacks>)
          Callbacks::station_connected(arg, event_data);
      default:
        break;
  }
}

template<typename Callbacks>
void
simple_wifi_retry<Callbacks>::ip_handler(void* arg,
                              std::int32_t event_id,
                              void* event_data) noexcept {
  retry_ = 0;
  event_.set(connected);
  if constexpr (wifi::detail::has_connected_v<Callbacks>)
    Callbacks::connected(arg, event_data);
}

template<typename Callbacks>
bool
simple_wifi_retry<Callbacks>::is_connected() const noexcept {
  return event_.is_set(connected);
}

template<typename Callbacks>
bool
simple_wifi_retry<Callbacks>::failed() const noexcept {
  return event_.is_set(fail);
}

template<typename Callbacks>
void register_handler(simple_wifi_retry<Callbacks>& instance) noexcept {
  wifi::register_handler(ESP_EVENT_ANY_ID, &simple_wifi_retry<Callbacks>::handler, &instance);
  sys::event::register_handler(IP_EVENT, IP_EVENT_STA_GOT_IP, &simple_wifi_retry<Callbacks>::handler, &instance);
}

}  // namespace station 
}  // namespace wifi


