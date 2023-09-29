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
#ifndef COMPONENT_OPTA_MAIN_PACKET_HPP__
#define COMPONENT_OPTA_MAIN_PACKET_HPP__

#include <cstdint>

#include "sys/error.hpp"
#include "http/server.hpp"
#include "websocket/server.hpp"

namespace ota {

enum class command : std::uint8_t {
  start = 1,
  state,
  abort,
  action,
  error = 10,
};

enum class error_code : std::uint8_t {
  success = 0,
  already_running,
  not_running,
  wrong_user,
  packet_size_error,
  no_command_found,
  no_action_found,
  action_wrong_time,
};

enum class abort_reason : std::uint8_t {
  no_abort = 0,
  user_request,
  user_disconnect,
  get_partiotion_error,
  invalid_version,
  same_version,
  timeout,
  ota_begin_error,
  ota_write_error,
  ota_end_error,
  set_partition_error,
};

enum class action : std::uint8_t {
  reset = 0,
  validate_image,
  invalidate_image,
};

#define ATTR_PACKED __attribute__((packed, aligned(1)))

struct error_packet {
  command     cmd;
  error_code  err;
} ATTR_PACKED;

struct start_request {
  command       cmd;
  std::uint32_t total_size;
  std::uint16_t timeout;      // max timeout to wait between packages
  std::uint8_t  reset:1;      // reset device after update succefully
  std::uint8_t  check_last_invalid:1;   // check if this version is the same of last invalid
  std::uint8_t  check_same_version:1;   // check if this version is the same of the current running;
  std::uint8_t  :5;
} ATTR_PACKED;

struct state_packet {
  command       cmd;
  std::uint32_t size_rcv;
  std::uint32_t size_request;
} ATTR_PACKED;

struct state_request {
  command       cmd;
  std::uint8_t  is_end:1;
  std::uint8_t  :7;
} ATTR_PACKED;

struct abort_request {
  command       cmd;
} ATTR_PACKED;

struct abort_packet {
  command       cmd;
  abort_reason  reason;
} ATTR_PACKED;

struct action_packet {
  command       cmd;
  action        act;
  std::int32_t  err;
} ATTR_PACKED;

struct action_request {
  command       cmd;
  action        act;
} ATTR_PACKED;

#undef ATTR_PACKED

#ifdef CONFIG_HTTPD_WS_SUPPORT

sys::error
send_error(websocket::client, error_code) noexcept;
sys::error
send_abort(websocket::client, abort_reason) noexcept;
sys::error
send_state(websocket::client,
           std::uint32_t size_rcv,
           std::uint32_t size_request) noexcept;
sys::error
send_action(websocket::client,
            action,
            std::int32_t) noexcept;

#endif  // CONFIG_HTTPD_WS_SUPPORT

}  // namespace ota

#endif // COMPONENT_OPTA_MAIN_PACKET_HPP__
