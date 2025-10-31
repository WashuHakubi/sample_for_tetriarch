/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once
#include <cstdint>
#include <type_traits>

namespace ew {
template <class T>
struct entity_traits;

template <>
struct entity_traits<uint32_t> {
  static constexpr uint32_t tombstone = UINT32_MAX;
  static constexpr uint32_t entities_per_page = 1024;
  static uint32_t to_index(uint32_t v) { return v; }
  static uint32_t next_entity(uint32_t cur) { return cur + 1; }
};

template <class T>
  requires(std::is_enum_v<T>)
struct entity_traits<T> : entity_traits<std::underlying_type_t<T>> {
  using underlying_type = std::underlying_type_t<T>;
  using base_type = entity_traits<underlying_type>;

  static uint32_t to_index(T v) { return base_type::to_index(static_cast<underlying_type>(v)); }
  static T next_entity(T cur) { return static_cast<T>(base_type::next_entity(static_cast<underlying_type>(cur))); }
};

enum class entity : uint32_t {};
} // namespace ew
