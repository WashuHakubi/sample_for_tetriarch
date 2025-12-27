/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <type_traits>

namespace ew {
template <class T>
  requires(std::is_scoped_enum_v<T>)
struct is_enum_flags_type : std::false_type {};

template <class T>
  requires(std::is_scoped_enum_v<T>)
constexpr bool is_enum_flags_type_v = is_enum_flags_type<T>::value;
} // namespace ew

template <class T>
  requires(ew::is_enum_flags_type_v<T>)
T operator~(T lhs) {
  return static_cast<T>(~static_cast<std::underlying_type_t<T>>(lhs));
}

template <class T>
  requires(ew::is_enum_flags_type_v<T>)
T operator&(T lhs, T rhs) {
  return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) & static_cast<std::underlying_type_t<T>>(rhs));
}

template <class T>
  requires(ew::is_enum_flags_type_v<T>)
T operator|(T lhs, T rhs) {
  return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) | static_cast<std::underlying_type_t<T>>(rhs));
}

template <class T>
  requires(ew::is_enum_flags_type_v<T>)
T operator^(T lhs, T rhs) {
  return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) ^ static_cast<std::underlying_type_t<T>>(rhs));
}

template <class T>
  requires(ew::is_enum_flags_type_v<T>)
T& operator&=(T& lhs, T rhs) {
  lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) & static_cast<std::underlying_type_t<T>>(rhs));
  return lhs;
}

template <class T>
  requires(ew::is_enum_flags_type_v<T>)
T& operator|=(T& lhs, T rhs) {
  lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) | static_cast<std::underlying_type_t<T>>(rhs));
  return lhs;
}

template <class T>
  requires(ew::is_enum_flags_type_v<T>)
T& operator^=(T& lhs, T rhs) {
  lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) ^ static_cast<std::underlying_type_t<T>>(rhs));
  return lhs;
}

template <class T>
  requires(ew::is_enum_flags_type_v<T>)
bool all_of(T value, T expected) {
  using ut = std::underlying_type_t<T>;
  return (static_cast<ut>(value) & static_cast<ut>(expected)) == static_cast<ut>(expected);
}

template <class T>
  requires(ew::is_enum_flags_type_v<T>)
bool any_of(T value, T expected) {
  using ut = std::underlying_type_t<T>;
  return (static_cast<ut>(value) | static_cast<ut>(expected)) != 0;
}
