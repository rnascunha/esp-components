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

#define OTA_WEBSOCKET_NAME_MAX_SIZE   (33)

namespace ota {

struct info {
  websocket::client   client{};
  http::server*       server;
  sys::task_handle    task = nullptr;
  char                image_name[OTA_WEBSOCKET_NAME_MAX_SIZE] = "";
};

sys::task_handle
wesocket_task(info&) noexcept;

}  // namespace OTA


#endif  // COMPONENTS_OTA_OTA_WEBSOCKET_HPP__