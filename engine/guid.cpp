/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "guid.h"

#ifdef _MSC_VER
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

#include <format>

namespace ewok {
Guid Guid::newGuid() {
  Guid result;
#ifdef _MSC_VER
  static_assert(sizeof(Guid) == sizeof(UUID));
  UuidCreate(reinterpret_cast<UUID*>(&result));
#else
  static_assert(sizeof(Guid) == sizeof(uuid_t));
  uuid_generate_random(reinterpret_cast<unsigned char*>(&result));
#endif
  return result;
}

Guid Guid::parse(std::string const& str) {
  Guid result;
#ifdef _MSC_VER
  UuidFromStringA(str.c_str(), reinterpret_cast<UUID*>(&result));
#else
  uuid_parse(str.c_str(), reinterpret_cast<unsigned char*>(&result));
#endif
  return result;
}

std::string Guid::toString() const {
  return std::format(
      "{:2x}{:2x}{:2x}{:2x}-{:2x}{:2x}-{:2x}{:2x}-{:2x}{:2x}-{:2x}{:2x}{:2x}{:2x}{:2x}{:2x}",
      bytes_[0],
      bytes_[1],
      bytes_[2],
      bytes_[3],
      bytes_[4],
      bytes_[5],
      bytes_[6],
      bytes_[7],
      bytes_[8],
      bytes_[9],
      bytes_[10],
      bytes_[11],
      bytes_[12],
      bytes_[13],
      bytes_[14],
      bytes_[15]);
}
} // namespace ewok
