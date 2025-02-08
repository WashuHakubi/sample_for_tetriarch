/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "guid.h"

#ifdef _WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

#include <inttypes.h>
#include <cassert>
#include <format>

namespace ewok {
Guid Guid::newGuid() {
  Guid result;
#ifdef _WIN32
  static_assert(sizeof(Guid) == sizeof(UUID));
  UuidCreate(reinterpret_cast<UUID*>(&result));
#else
  static_assert(sizeof(Guid) == sizeof(uuid_t));
  uuid_generate_random(reinterpret_cast<unsigned char*>(&result));
#endif
  return result;
}

Guid Guid::parse(std::string const& str) {
  Guid guid;

#define G32 "%8" SCNx32
#define G16 "%4" SCNx16
#define G8 "%2" SCNx8

  int nchars = -1;
  int nfields = sscanf(
      str.c_str(),
      G32 "-" G16 "-" G16 "-" G8 G8 "-" G8 G8 G8 G8 G8 G8 "%n",
      &guid.a_,
      &guid.b_,
      &guid.c_,
      &guid.d_[0],
      &guid.d_[1],
      &guid.d_[2],
      &guid.d_[3],
      &guid.d_[4],
      &guid.d_[5],
      &guid.d_[6],
      &guid.d_[7],
      &nchars);

  assert(nfields == 11 && nchars == 36);
#undef G8
#undef G16
#undef G32

  return guid;
}

std::string Guid::toString() const {
  return std::format(
      "{:8x}-{:4x}-{:4x}-{:2x}{:2x}-{:2x}{:2x}{:2x}{:2x}{:2x}{:2x}",
      a_,
      b_,
      c_,
      d_[0],
      d_[1],
      d_[2],
      d_[3],
      d_[4],
      d_[5],
      d_[6],
      d_[7]);
}
} // namespace ewok
