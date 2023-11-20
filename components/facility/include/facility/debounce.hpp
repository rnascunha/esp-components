#ifndef COMPONENTS_FACILITY_DEBOUNCE_HPP_
#define COMPONENTS_FACILITY_DEBOUNCE_HPP_

#include <cassert>

#include "sys/sys.hpp"
#include "uc/gpio.hpp"

namespace facility {

struct debounce {
  constexpr
  debounce(uc::gpio&& button, int limit) noexcept
   : btn_(button), limit_(limit) {
    assert(limit > 0 && "Limit must be positive");
   }

  constexpr void
  reset() noexcept {
    count_ = 0;
  }

  bool
  check() noexcept {
    if (btn_.read() == 0) {
      if (++count_ == limit_) {
        count_ = 0;
        return true;
      }
    } else
      count_ = 0;
    return false;
  }

  template<typename Duration,
           typename Callable,
           typename ...Args>
  void wait(Duration&& dt, Callable&& call, Args&&... args) noexcept {
    while(true) {
      if (check()) {
        call(std::forward<Args>(args)...);
        break;
      }
      sys::delay(dt);
    }
  }

 private:
  uc::gpio  btn_;
  int limit_;
  int count_ = 0;
};

template<sys::Duration D, typename Callback, int LevelPress = 0>
struct debounce_isr {
  debounce_isr(uc::gpio&& gpio, D d, Callback cb, void* arg = nullptr) noexcept
    : btn(std::move(gpio)), duration(d), cb(cb) {
    btn.add_isr(GPIO_INTR_ANYEDGE, [](void* arg) {
      auto& b = *static_cast<debounce_isr*>(arg);
      if (b.btn.read() == LevelPress) {
        b.press_time = sys::time::uptime<std::chrono::microseconds>();
      } else {
        if (sys::time::uptime<std::chrono::microseconds>() - b.press_time > b.duration) {
          b.cb(b.arg);
        }
      }
    }, this);
  }

  uc::gpio btn;
  D duration;
  Callback cb;
  void* arg = nullptr;
  std::chrono::microseconds press_time = std::chrono::microseconds(0);
};

}  // namespace facility

#endif  // COMPONENTS_FACILITY_DEBOUNCE_HPP_