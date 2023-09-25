#include "esp_netif.h"

#include "sys/error.hpp"

namespace sys {
namespace net {

esp_netif_ip_info_t ip(esp_netif_t* handler) noexcept {
  esp_netif_ip_info_t ip_info{};
  esp_netif_get_ip_info(handler, &ip_info);
  return ip_info;
}

error 
ip(esp_netif_t* handler, const esp_netif_ip_info_t& ip) noexcept {
  return esp_netif_set_ip_info(handler, &ip);
}

}  // namespace net
}  // namespace sys
