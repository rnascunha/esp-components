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
send_state(bool is_end, std::uint32_t size_rcv, http::server& server) noexcept {
  return websocket::send_all(server, state_packet{
    .cmd = command::state,
    .finished = is_end,
    .size_rcv = size_rcv
  });
}

}  // namespace ota
