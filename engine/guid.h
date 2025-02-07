/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <compare>
#include <cstdint>
#include <string>

namespace ewok {
class alignas(uint32_t) Guid {
 public:
  static Guid newGuid();
  static Guid parse(std::string const& str);

  std::string toString() const;

  auto operator<=>(Guid const&) const = default;

 private:
  uint8_t bytes_[16]{};
};
} // namespace ewok

namespace std {
template <>
struct hash<ewok::Guid> {
  auto operator()(ewok::Guid const& guid) const {
    uint64_t r = 0;

    auto p = reinterpret_cast<uint32_t const*>(&guid);

    r = mx(r + p[0]);
    r = mx(r + p[1]);
    r = mx(r + p[2]);
    r = mx(r + p[3]);

    return static_cast<size_t>(fmx(r));
  }

  // Borrowed from boost::uuid https://github.com/boostorg/uuid
  static constexpr uint64_t mx(uint64_t x) {
    x *= 0xD96AAA55;
    x ^= x >> 16;
    return x;
  }

  // Borrowed from boost::uuid https://github.com/boostorg/uuid
  static constexpr uint64_t fmx(uint64_t x) {
    x *= 0x7DF954AB;
    x ^= x >> 16;
    return x;
  }
};
} // namespace std