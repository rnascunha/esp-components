/**
 * @file websocket.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_OTA_OTA_WEBSOCKET_HPP__
#define COMPONENTS_OTA_OTA_WEBSOCKET_HPP__

#include "esp_app_format.h"
#include "esp_app_desc.h"

#include "websocket/server.hpp"
#include "packet.hpp"

namespace ota {

namespace detail {
  static constexpr const
  std::size_t header_size = sizeof(esp_image_header_t) +
                            sizeof(esp_image_segment_header_t) +
                            sizeof(esp_app_desc_t);
}  // namespace detail

template<std::size_t Size>
struct info {
  static_assert(Size >= detail::header_size + sizeof(state_request),
                "Buffer to update image must be bigger");

  static constexpr const std::size_t buffer_size = Size;

  websocket::client       client{};
  http::server*           server;
  sys::task_handle        task = nullptr;
  std::uint32_t           file_size = 0;
  std::uint16_t           timeout;                // max timeout to wait between packages
  std::uint8_t            reset:1;                // reset device after update succefully
  std::uint8_t            check_last_invalid:1;   // check if this version is the same of last invalid
  std::uint8_t            check_same_version:1;   // check if this version is the same of the current running;
  std::uint8_t            :5;
  websocket::frame        frame{};
  std::uint8_t            buffer[Size];
};

template<std::size_t Size>
struct ws_cb {
  inline static ota::info<Size> info{};

  static sys::error on_data(websocket::request) noexcept;
  static void on_close (int, void*) noexcept;

  static sys::error start(websocket::request) noexcept;
  static sys::error state(websocket::request) noexcept;
  static sys::error abort(websocket::request) noexcept;
};

}  // namespace ota

#include "impl/websocket.ipp"

#endif  // COMPONENTS_OTA_OTA_WEBSOCKET_HPP__