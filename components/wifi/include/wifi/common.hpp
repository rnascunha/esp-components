/**
 * @file common.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_WIFI_COMMON_HPP_
#define COMPONENTS_WIFI_COMMON_HPP_

#include <cstdint>

#include "esp_wifi_types.h"
#include "esp_event.h"

#include "sys/error.hpp"

namespace wifi {

using config = wifi_config_t;

sys::error start() noexcept;
sys::error connect() noexcept;

sys::error register_handler(std::int32_t,
                            esp_event_handler_t,
                            void* = nullptr) noexcept;

// tags used through the application
struct not_register{};
struct no_callback{};

}  // namespace wifi

#endif  // COMPONENTS_WIFI_COMMON_HPP_