/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <bitset>
#include <climits>
#include <cstdint>

namespace wut {
template <class T, size_t N = sizeof(uint32_t) * CHAR_BIT>
struct Flags {
  constexpr bool test(T bit) const { return bits_.test(to_underlying(bit)); }

  constexpr auto& set(T bit, bool value = true) {
    bits_.set(to_underlying(bit), value);
    return *this;
  }

 private:
  constexpr auto to_underlying(T val) const { return static_cast<std::underlying_type_t<T>>(val); }

  std::bitset<N> bits_;
};
} // namespace wut
