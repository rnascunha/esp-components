#ifndef COMPONENTS_FACILITY_DEBOUNCE_HPP_
#define COMPONENTS_FACILITY_DEBOUNCE_HPP_

#include <cassert>

#include "sys/sys.hpp"
#include "sys/time.hpp"
#include "sys/timer.hpp"
#include "uc/gpio.hpp"

namespace facility {

template<sys::Duration D, int LevelPress = 0>
struct debounce {
  constexpr
  debounce(uc::gpio&& button, D duration) noexcept
   : btn_(std::move(button)), duration_(duration) {}

  bool
  check() noexcept {
    if (press_time_ == std::chrono::microseconds(0) &&
        btn_.read() == LevelPress) {
      press_time_ = sys::time::uptime<std::chrono::microseconds>();
    } else if (press_time_ != std::chrono::microseconds(0) && 
               sys::time::uptime<std::chrono::microseconds>() - press_time_ > duration_) {
      press_time_ = std::chrono::microseconds(0);
      return true;
    }
    return false;
  }

  template<sys::time::tick_time Duration,
           typename Callable,
           typename ...Args>
  void wait(Duration&& interval_check, Callable&& call, Args&&... args) noexcept {
    while(true) {
      if (check()) {
        call(std::forward<Args>(args)...);
        break;
      }
      sys::delay(interval_check);
    }
  }

 private:
  uc::gpio  btn_;
  D duration_;
  std::chrono::microseconds press_time_ = std::chrono::microseconds(0);
};

template<sys::Duration D, typename Callback, int LevelPress = 0>
struct debounce_isr {
  debounce_isr(uc::gpio&& gpio, D d, Callback cb, void* arg = nullptr) noexcept
    : btn(std::move(gpio)), duration(d), cb(cb) {
    btn.add_isr(GPIO_INTR_ANYEDGE, [](void* arg) {
      auto& b = *static_cast<debounce_isr*>(arg);
      if (b.btn.read() == LevelPress) {
        b.press_time = sys::time::uptime<std::chrono::microseconds>();
      } else if (sys::time::uptime<std::chrono::microseconds>() - b.press_time > b.duration) {
          b.cb(b.arg);
      }
    }, this);
  }

  uc::gpio btn;
  D duration;
  Callback cb;
  void* arg = nullptr;
  std::chrono::microseconds press_time = std::chrono::microseconds(0);
};

template<sys::Duration D, typename Callback, int LevelPress = 0>
struct debounce_timer {
  debounce_timer(uc::gpio&& gpio, D d, Callback cb, void* arg = nullptr) noexcept
   : btn(std::move(gpio)),
     duration(d),
     timer(esp_timer_create_args_t{
      .callback = [](void* arg) {
        auto& b = *static_cast<debounce_timer*>(arg);
        if (b.press_time == std::chrono::microseconds(0) &&
            b.btn.read() == LevelPress) {
          b.press_time = sys::time::uptime<std::chrono::microseconds>();
        } else if (b.press_time != std::chrono::microseconds(0) && 
                  sys::time::uptime<std::chrono::microseconds>() - b.press_time > b.duration) {
          b.press_time = std::chrono::microseconds(0);
          b.cb(b.arg);
        }
      },
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "",
      .skip_unhandled_events = true, 
   }), cb(cb), arg(arg) {}
    
  void start(sys::Duration auto interval_check) {
    timer.start_periodic(interval_check);
  }

  uc::gpio btn;
  D duration;
  sys::timer timer;
  Callback cb;
  void* arg = nullptr;
  std::chrono::microseconds press_time = std::chrono::microseconds(0);
};

}  // namespace facility

#endif  // COMPONENTS_FACILITY_DEBOUNCE_HPP_