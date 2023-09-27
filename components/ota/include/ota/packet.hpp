/**
 * @file packet.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MAIN_PACKET_HPP__
#define MAIN_PACKET_HPP__

#include <cstdint>

#include "sys/error.hpp"
#include "http/server.hpp"
#include "websocket/server.hpp"

namespace ota {

enum class command : std::uint8_t {
  start = 1,
  state,
  abort,
  error = 10,
};

enum class error_code : std::uint8_t {
  already_running = 1,
  not_running,
  wrong_user,
  name_too_long
};

enum class abort_reason : std::uint8_t {
  user_request = 1,
  user_disconnect
};

#define ATTR_PACKED __attribute__((packed, aligned(1)))

struct error_packet {
  command     cmd;
  error_code  err;
} ATTR_PACKED;

struct start_request {
  command       cmd;
  std::uint32_t total_size;
  const char*   name;
} ATTR_PACKED;

struct state_packet {
  command       cmd;
  std::uint8_t  finished:1;
  std::uint8_t  :7;     // no use
  std::uint32_t size_rcv;
};

struct abort_request {
  command       cmd;
} ATTR_PACKED;

struct abort_packet {
  command       cmd;
  abort_reason  reason;
} ATTR_PACKED;

#undef ATTR_PACKED

sys::error
send_error(websocket::client, error_code) noexcept;
sys::error
send_abort(abort_reason, http::server&) noexcept;
sys::error
send_state(bool is_end, std::uint32_t size_rcv, http::server&) noexcept;

}  // namespace ota

#endif // MAIN_PACKET_HPP__
