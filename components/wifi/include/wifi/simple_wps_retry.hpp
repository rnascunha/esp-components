/**
 * @file simple_wps_retry.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-04-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_WIFI_SIMPLE_WPS_RETRY_HANDLER_HPP_
#define COMPONENTS_WIFI_SIMPLE_WPS_RETRY_HANDLER_HPP_

#include <limits>
#include <cstdint>

#include "esp_wps.h"

#include "sys/event.hpp"
#include "sys/time.hpp"
#include "sys/task.hpp"

namespace wifi {
namespace station {

template<typename Callbacks = wifi::no_callback>
class simple_wps_retry {
 public:
  static constexpr const unsigned connected = BIT0;
  static constexpr const unsigned fail = BIT1;

  simple_wps_retry(int max_retry = std::numeric_limits<int>::max()) noexcept;
  simple_wps_retry(wifi::not_register, int max_retry = std::numeric_limits<int>::max()) noexcept;

  ~simple_wps_retry() noexcept;

  sys::error start() noexcept;

  EventBits_t
  wait() noexcept {
    return event_.wait(connected | fail,
                       sys::time::max,
                       pdFALSE,
                       pdFALSE);
  }

  EventBits_t
  wait(sys::time::tick_time auto duration) noexcept {
    return event_.wait(connected | fail,
                       duration,
                       pdFALSE,
                       pdFALSE);
  }

  static void handler(void* arg,
               esp_event_base_t event_base,
               std::int32_t event_id,
               void* event_data) noexcept;

  int retry() const noexcept { return retry_; }
  int max_retry() const noexcept { return max_retry_; }
  void reset() noexcept;

  bool is_connected() const noexcept;
  bool failed() const noexcept;
 private:
  void wifi_handler(void* arg,
                    std::int32_t event_id,
                    void* event_data) noexcept;
  void ip_handler(void* arg,
                    std::int32_t event_id,
                    void* event_data) noexcept;

  sys::event_group event_{};
  int max_retry_;
  int retry_ = 0;
  int ap_idx_ = 0;
  esp_wps_config_t config_ = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PBC);
  wifi_config_t wps_ap_creds_[MAX_WPS_AP_CRED];
  int ap_creds_num_ = 0;
};

template<typename Callbacks>
void register_handler(simple_wps_retry<Callbacks>& instance) noexcept;

}  // namespace station 
}  // namespace wifi

#include "impl/simple_wps_retry.ipp"

#endif //  COMPONENTS_WIFI_SIMPLE_WPS_RETRY_HANDLER_HPP_
