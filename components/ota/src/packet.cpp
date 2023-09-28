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

sys::error
send_error(websocket::client client, error_code code) noexcept {
  return client.send(error_packet{
    .cmd = command::error,
    .err = code
  });
}

sys::error
send_abort(abort_reason reason, http::server& server) noexcept {
  return websocket::send_all(server, abort_packet{
    .cmd = command::abort,
    .reason = reason
  });
}

sys::error
send_state(std::uint32_t size_rcv, http::server& server) noexcept {
  return websocket::send_all(server, state_packet{
    .cmd = command::state,
    .size_rcv = size_rcv,
    .size_request = 0
  });
}

sys::error
send_state(websocket::client& client, std::uint32_t size_rcv, std::uint32_t size_request) noexcept {
  return client.send(state_packet{
    .cmd = command::state,
    .size_rcv = size_rcv,
    .size_request = size_request
  });
}

}  // namespace ota
