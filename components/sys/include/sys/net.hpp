/**
 * @file net.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_SYS_SYS_NET_HPP__
#define COMPONENTS_SYS_SYS_NET_HPP__

#include "esp_netif.h"

#include "sys/error.hpp"

namespace sys {
namespace net {

inline error
dhcp_client_stop(esp_netif_t* net_handler) noexcept {
  return esp_netif_dhcpc_stop(net_handler);
}

esp_netif_ip_info_t ip(esp_netif_t*) noexcept;
error ip(esp_netif_t*, const esp_netif_ip_info_t&) noexcept;

}  // net
}  // sys

#endif // COMPONENTS_SYS_SYS_NET_HPP__
