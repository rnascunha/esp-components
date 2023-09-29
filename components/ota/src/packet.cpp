/**
 * @file packet.cpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "sys/error.hpp"

#include "ota/packet.hpp"
#include "websocket/server.hpp"

namespace ota {

#ifdef CONFIG_HTTPD_WS_SUPPORT

sys::error
send_error(websocket::client client, error_code code) noexcept {
  return client.send(error_packet{
    .cmd = command::error,
    .err = code
  });
}

sys::error
send_abort(websocket::client client, abort_reason reason) noexcept {
  return client.send(abort_packet{
    .cmd = command::abort,
    .reason = reason
  });
}

sys::error
send_state(websocket::client client, std::uint32_t size_rcv, std::uint32_t size_request) noexcept {
  return client.send(state_packet{
    .cmd = command::state,
    .size_rcv = size_rcv,
    .size_request = size_request
  });
}

#endif  // CONFIG_HTTPD_WS_SUPPORT

}  // namespace ota
