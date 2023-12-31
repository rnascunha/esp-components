/**
 * @file server.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-06
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_WEBSOCKET_SERVER_HPP_
#define COMPONENTS_WEBSOCKET_SERVER_HPP_

#include "sdkconfig.h"

#ifdef CONFIG_HTTPD_WS_SUPPORT

#include <type_traits>
#include <memory>
#include <span>

#include "esp_http_server.h"

#include "sys/error.hpp"

#include "http/server.hpp"
#include "http/server_connect_cb.hpp"

#include "detail/type_traits.hpp"

namespace websocket {

template<typename Func>
static esp_err_t ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    if constexpr (detail::has_on_open_v<Func>)
      return Func::on_open(req);
    else
      return ESP_OK;
  }
  return Func::on_data(req);
}

template<typename Func>
void event_handler(void* event_handler_arg,
                    esp_event_base_t event_base,
                    std::int32_t event_id,
                    void* event_data) {
  Func::on_close(*(int*) event_data, event_handler_arg);
}

template<typename Func>
sys::error
register_handler(void* user_ctx = nullptr) {
  if constexpr (detail::has_on_close_v<Func>) {
    http::unregister_handler(HTTP_SERVER_EVENT_DISCONNECTED,
                          &event_handler<Func>);
    return http::register_handler(HTTP_SERVER_EVENT_DISCONNECTED,
                                  &event_handler<Func>,
                                  user_ctx);
  } else
    return ESP_OK;
}

template<typename Func>
struct uri {
  static_assert(detail::has_on_data_v<Func>,
                  "on_data static method must be defined");

  const char* uri = "/";
  void*       user_ctx = nullptr;
  bool        control_frames = false;
  const char* supported_subprotocol = nullptr;

  http::server::uri operator()() noexcept {
    if constexpr (detail::has_on_close_v<Func>) {
      register_handler<Func>(user_ctx);
    }
    return http::server::uri{
      .uri          = uri,
      .method       = HTTP_GET,
      .handler      = ws_handler<Func>,
      .user_ctx     = user_ctx,
      .is_websocket = true,
      .handle_ws_control_frames = control_frames,
      .supported_subprotocol = supported_subprotocol
    };
  }
};

using frame = httpd_ws_frame_t;

struct data {
  frame packet{};
  std::unique_ptr<std::uint8_t[]> buffer = nullptr;
};

class request {
 public:
  request(httpd_req_t* req) noexcept;

  sys::error
  receive(frame& frame,
          std::span<std::uint8_t> buffer) noexcept;
  sys::error
  receive(data& d) noexcept;

  sys::error
  send(frame&) noexcept;
  sys::error
  send(data&) noexcept;

  void* context() noexcept {
    return native()->user_ctx;
  }

  [[nodiscard]] int
  socket() noexcept;
  [[nodiscard]] httpd_req_t*
  native() noexcept;
  [[nodiscard]] httpd_handle_t
  handler() noexcept;

 private:
  httpd_req_t* req_;
};

struct client {
  client() = default;
  client(httpd_handle_t, int) noexcept;
  client(request&) noexcept;

  sys::error send(frame&) noexcept;
  sys::error send(data&) noexcept;

  template<typename T, std::size_t N>
  sys::error send(std::span<const T, N> const data,
                  httpd_ws_type_t type = HTTPD_WS_TYPE_BINARY) noexcept {
    frame frm = {
      .payload = data.data(),
      .len = data.size_bytes(),
      .type = type
    };
    return send(frm);
  }

  template<typename T>
  sys::error send(const T& data, 
                  httpd_ws_type_t type = HTTPD_WS_TYPE_BINARY) noexcept {
    frame frm = {};
    frm.payload = (std::uint8_t*)&data,
    frm.len = sizeof(T),
    frm.type = type;

    return send(frm);
  }

  bool is_valid() const noexcept {
    return hd != nullptr;
  }

  void clear() noexcept {
    hd = nullptr;
    fd = 0;  
  }

  void close() noexcept {
    httpd_sess_trigger_close(hd, fd);
    hd = nullptr;
    fd = 0;
  }

  bool operator==(const client& c) const noexcept {
    return hd == c.hd && fd == c.fd;
  }

  bool operator!=(const client& c) const noexcept {
    return !(*this == c);
  }

  httpd_handle_t hd = nullptr;
  int            fd = 0;
};

template<typename T>
sys::error
send_all(http::server& server,
         const T& packet, 
         httpd_ws_type_t type = HTTPD_WS_TYPE_BINARY) noexcept {
  std::size_t size = 7;
  int clients[7];
  auto err = server.client_list(size, clients);
  if (err) return err;
  if (size == 0)
    return err;
  
  websocket::frame pkt{};
  
  pkt.payload = (std::uint8_t*)&packet;
  pkt.len = sizeof(packet);
  pkt.type = type;

  for (int i = 0; i < size; ++i)
    websocket::client(server.native(), clients[i]).send(pkt);

  return err;
};

sys::error
queue(client&, httpd_work_fn_t, void* = nullptr) noexcept;
sys::error
queue(request&, httpd_work_fn_t, void* = nullptr) noexcept;

}  // namespace websocket

#endif  // CONFIG_HTTPD_WS_SUPPORT

#endif  // COMPONENTS_WEBSOCKET_SERVER_HPP_
