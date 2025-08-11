/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <random>

namespace ewok::server {
struct Random {
  static uint32_t next(uint32_t minInclusive, uint32_t maxInclusive) {
    return std::uniform_int_distribution{minInclusive, maxInclusive}(s_gen);
  }

  static float nextReal(float minInclusive = 0, float maxExclusive = 1) {
    return std::uniform_real_distribution{minInclusive, maxExclusive}(s_gen);
  }

private:
  static std::mt19937 s_gen;
};
}