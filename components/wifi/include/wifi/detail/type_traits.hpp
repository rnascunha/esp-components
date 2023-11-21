/**
 * @file type_traits.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_WIFI_DETAIL_TYPE_TRAITS_HPP_
#define COMPONENTS_WIFI_DETAIL_TYPE_TRAITS_HPP_

#include <type_traits>

namespace wifi {
namespace detail {

template<class Func, class = void>
struct has_station_connected : std::false_type{};

template<class Func>
struct has_station_connected<Func,
                   std::enable_if_t<
                    std::is_invocable_v<
                      decltype(Func::station_connected),
                      void*, void*>
                    >
                  > : std::true_type{};

template<class Func>
static constexpr const bool
has_station_connected_v = has_station_connected<Func>::value;

template<class Func, class = void>
struct has_connected : std::false_type{};

template<class Func>
struct has_connected<Func,
                   std::enable_if_t<
                    std::is_invocable_v<
                      decltype(Func::connected),
                      void*, void*>
                    >
                  > : std::true_type{};

template<class Func>
static constexpr const bool
has_connected_v = has_connected<Func>::value;

template<class Func, class = void>
struct has_connecting : std::false_type{};

template<class Func>
struct has_connecting<Func,
                   std::enable_if_t<
                    std::is_invocable_v<
                      decltype(Func::connecting),
                      void*, void*>
                    >
                  > : std::true_type{};

template<class Func>
static constexpr const bool
has_connecting_v = has_connecting<Func>::value;

template<class Func, class = void>
struct has_disconnected : std::false_type{};

template<class Func>
struct has_disconnected<Func,
                   std::enable_if_t<
                    std::is_invocable_v<
                      decltype(Func::disconnected),
                      void*, void*>
                    >
                  > : std::true_type{};

template<class Func>
static constexpr const bool
has_disconnected_v = has_disconnected<Func>::value;

template<class Func, class = void>
struct has_fail : std::false_type{};

template<class Func>
struct has_fail<Func,
                   std::enable_if_t<
                    std::is_invocable_v<
                      decltype(Func::fail),
                      void*, void*>
                    >
                  > : std::true_type{};

template<class Func>
static constexpr const bool
has_fail_v = has_fail<Func>::value;

}  // namespace detail
}  // namespace wifi

#endif  // COMPONENTS_WIFI_DETAIL_TYPE_TRAITS_HPP_
