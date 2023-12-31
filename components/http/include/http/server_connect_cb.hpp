/**
 * @file simple_connect.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-04-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_HTTP_SERVER_CONNECT_CB_HPP_
#define COMPONENTS_HTTP_SERVER_CONNECT_CB_HPP_

#include <cstdint>
#include <type_traits>

#include "esp_wifi.h"

#include "esp_http_server.h"
#if CONFIG_ESP_HTTPS_SERVER_ENABLE == 1
#include "esp_https_server.h"
#endif  // CONFIG_ESP_HTTPS_SERVER_ENABLE == 1

#include "sys/event.hpp"
#include "http/server.hpp"
#include "http/functions.hpp"

namespace http {

struct not_register{};

using server_cb_func = void(*)(server&);

template<bool IsSecure = false,
         typename StartCallable = server_cb_func>
struct server_connect_cb {
  static constexpr const bool is_secure = IsSecure;
#if CONFIG_ESP_HTTPS_SERVER_ENABLE == 1
  using config_type = std::conditional_t<IsSecure,
                                         server::ssl_config,
                                         server::config>;
#else
  using config_type = server::config;
#endif //  CONFIG_ESP_HTTPS_SERVER_ENABLE == 1

  server_connect_cb(StartCallable&& callable) noexcept;
  server_connect_cb(not_register, StartCallable&& callable) noexcept
   : call{callable} {}

  sys::error start() noexcept {
    if (!svr.is_connected()) {
      auto err = svr.start(config);
      if (!err) call(svr);
      return err;
    }
    return ESP_ERR_HTTPD_HANDLER_EXISTS;
  }

  server        svr;
  config_type   config = default_config<IsSecure>();
  StartCallable call;
};

template<bool IsSecure,
         typename Callable>
static void server_disconnect(void* arg,
                              esp_event_base_t,
                              std::int32_t, void*) {
  using type = server_connect_cb<IsSecure, Callable>;
#if CONFIG_ESP_HTTPS_SERVER_ENABLE == 1
  ((type*)arg)->svr.template stop<type::is_secure>();
#else
  ((type*)arg)->svr.stop();
#endif  // CONFIG_ESP_HTTPS_SERVER_ENABLE == 1
}

template<bool IsSecure,
         typename Callable>
static void server_connect(void* arg,
                           esp_event_base_t,
                           std::int32_t, void*){
  ((server_connect_cb<IsSecure, Callable>*)arg)->start();
}

template<bool IsSecure,
         typename Callable>
void register_handler(server_connect_cb<IsSecure, Callable>& server) noexcept {
  sys::event::register_handler(IP_EVENT, IP_EVENT_STA_GOT_IP, &server_connect<IsSecure, Callable>, &server);
  sys::event::register_handler(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &server_disconnect<IsSecure, Callable>, &server);
}

template<bool IsSecure,
         typename Callable>
server_connect_cb<IsSecure, Callable>::
server_connect_cb(Callable&& callable) noexcept
: call{callable} {
  register_handler(*this);
}

template<bool IsSecure, typename StartCallable>
server_connect_cb(StartCallable&&) -> server_connect_cb<IsSecure, StartCallable>;

}  // namespace http

#endif  // COMPONENTS_HTTP_SERVER_CONNECT_CB_HPP_
