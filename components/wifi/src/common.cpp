#include "esp_netif.h"
#include "esp_wifi.h"

#include "sys/error.hpp"
#include "sys/event.hpp"

namespace wifi {

sys::error start() noexcept {
  return esp_wifi_start();
}

sys::error connect() noexcept {
  return esp_wifi_connect();
}

sys::error register_handler(std::int32_t id,
                            esp_event_handler_t handler,
                            void* arg /* = nullptr */) noexcept {
  return sys::event::register_handler(WIFI_EVENT, id, handler, arg);
}

}  // namespace wifi
