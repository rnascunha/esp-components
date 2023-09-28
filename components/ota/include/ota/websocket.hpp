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

#include "websocket/server.hpp"

namespace ota {

template<std::size_t Size>
struct info {
  static constexpr const std::size_t buffer_size = Size;

  websocket::client       client{};
  http::server*           server;
  sys::task_handle        task = nullptr;
  std::uint32_t           file_size = 0;
  websocket::frame        frame{};
  std::uint8_t            buffer[Size];
};

template<std::size_t Size>
sys::task_handle
wesocket_task(info<Size>&) noexcept;

template<std::size_t Size>
struct ws_cb {
  inline static ota::info<Size> info{};

  static void on_close (int, void*) noexcept;
  static sys::error on_data(websocket::request) noexcept;

  static sys::error start(websocket::request) noexcept;
  static sys::error state(websocket::request) noexcept;
  static sys::error abort(websocket::request) noexcept;
};

}  // namespace ota

#include "impl/websocket.ipp"


#endif  // COMPONENTS_OTA_OTA_WEBSOCKET_HPP__