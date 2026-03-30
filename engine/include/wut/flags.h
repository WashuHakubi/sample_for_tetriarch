/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace wut {
template <class T, class Store = uint32_t>
  requires(std::is_enum_v<T> && std::is_integral_v<Store>)
struct Flags {
  Flags() = default;

  Flags(T value) { set(value); }

  /**
   * Sets the flags to exactly value.
   */
  Flags(Store value) : bits_(value) {}

  constexpr auto& clear(T position) {
    set(position, false);
    return *this;
  }

  constexpr auto& set(T position, bool value = true) {
    auto mask = 1 << to_underlying(position);
    if (value) {
      bits_ |= mask;
    } else {
      bits_ &= ~mask;
    }
    return *this;
  }

  constexpr bool test(T position) const {
    auto mask = 1 << to_underlying(position);
    return (bits_ & mask) != 0;
  }

  constexpr auto value() const { return bits_; }

  constexpr bool any(Flags const& other) const { return (bits_ & other.bits_) != 0; }

  constexpr bool all(Flags const& other) const { return (bits_ & other.bits_) == other.bits_; }

  template <T... Args>
  constexpr bool any() const {
    auto mask = ((1 << to_underlying(Args)) | ...);
    return (bits_ & mask) != 0;
  }

  template <T... Args>
  constexpr bool all() const {
    auto mask = ((1 << to_underlying(Args)) | ...);
    return (bits_ & mask) == mask;
  }

  constexpr Flags& operator|=(Flags const& other) {
    bits_ |= other.bits_;
    return *this;
  }

  constexpr Flags& operator&=(Flags const& other) {
    bits_ &= other.bits_;
    return *this;
  }

  constexpr bool operator==(Flags const& other) const { return bits_ == other.bits_; }

  constexpr bool operator!=(Flags const& other) const { return bits_ != other.bits_; }

 private:
  constexpr auto to_underlying(T val) const { return static_cast<std::underlying_type_t<T>>(val); }

  Store bits_;
};

template <class T, class Store>
Flags<T, Store> operator|(Flags<T, Store> const& lhs, Flags<T, Store> const& rhs) {
  auto result = lhs;
  lhs |= rhs;
  return result;
}

template <class T, class Store>
Flags<T, Store> operator|(Flags<T, Store> const& lhs, T rhs) {
  auto result = lhs;
  result |= rhs;
  return result;
}

template <class T, class Store>
Flags<T, Store> operator&(Flags<T, Store> const& lhs, Flags<T, Store> const& rhs) {
  auto result = lhs;
  lhs |= rhs;
  return result;
}

template <class T, class Store>
Flags<T, Store> operator&(Flags<T, Store> const& lhs, T rhs) {
  auto result = lhs;
  result &= rhs;
  return result;
}

template <class T, class Store>
Flags<T, Store> operator~(Flags<T, Store> const& lhs) {
  auto result = lhs;
  return result.value();
}
} // namespace wut
