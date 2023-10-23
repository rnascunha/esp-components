/**
 * @file retry_handler.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-04-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_WIFI_SIMPLE_RETRY_HANDLER_HPP_
#define COMPONENTS_WIFI_SIMPLE_RETRY_HANDLER_HPP_

#include <limits>
#include <cstdint>

#include "sys/event.hpp"
#include "sys/time.hpp"
#include "sys/task.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

namespace wifi {
namespace station {

template<typename Callbacks = wifi::no_callback>
class simple_wifi_retry {
 public:
  static constexpr const unsigned connected = BIT0;
  static constexpr const unsigned fail = BIT1;

  simple_wifi_retry(int max_retry = std::numeric_limits<int>::max()) noexcept;
  simple_wifi_retry(wifi::not_register, int max_retry = std::numeric_limits<int>::max()) noexcept;

  ~simple_wifi_retry() noexcept;

  EventBits_t
  wait() noexcept;
  
  EventBits_t
  wait(sys::time::tick_time auto) noexcept;

  static void handler(void* arg,
               esp_event_base_t event_base,
               std::int32_t event_id,
               void* event_data) noexcept;

  int retry() const noexcept { return retry_; }
  void reset() noexcept;

  int max_retry() const noexcept { return max_retry_; }
  void max_retry(int retry_num) noexcept { max_retry_ = retry_num; }

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
};

template<typename Callbacks>
void register_handler(simple_wifi_retry<Callbacks>& instance) noexcept;

}  // namespace station 
}  // namespace wifi

#include "impl/simple_wifi_retry.ipp"

#endif //  COMPONENTS_WIFI_SIMPLE_RETRY_HANDLER_HPP_
