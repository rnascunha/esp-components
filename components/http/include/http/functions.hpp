/**
 * @file helper.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_HTTP_HELPER_HPP_
#define COMPONENTS_HTTP_HELPER_HPP_

#include <variant>
#include <array>

#include "esp_http_server.h"

#include "http/server.hpp"

namespace http {

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

using uri_error = std::variant<server::uri, server::error>;

template<unsigned int N>
void
register_uris(server& handler, const std::array<uri_error, N>& uris) noexcept {
  for (const auto& uri : uris) {
    std::visit(overloaded{
      [&handler](const auto& uuri) { handler.register_uri(uuri); }
    }, uri);
  }
}

#if CONFIG_ESP_HTTPS_SERVER_ENABLE == 1
template<bool IsSecure>
[[nodiscard]] constexpr auto
default_config() noexcept -> std::conditional_t<IsSecure,
                                         server::ssl_config,
                                         server::config> {
  if constexpr (IsSecure)
    return HTTPD_SSL_CONFIG_DEFAULT();
  else
    return HTTPD_DEFAULT_CONFIG();
}
#else
template<bool IsSecure>
[[nodiscard]] constexpr auto
default_config() noexcept -> server::config {
  return HTTPD_DEFAULT_CONFIG();
}
#endif  // CONFIG_ESP_HTTPS_SERVER_ENABLE == 1

}  // namespace http

#endif  // COMPONENTS_HTTP_HELPER_HPP_
