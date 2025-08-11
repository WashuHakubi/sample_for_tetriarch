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
  static uint32_t next(uint32_t min, uint32_t max) { return std::uniform_int_distribution{min, max}(s_gen); }

  static float nextReal(float min = 0, float max = 1) { return std::uniform_real_distribution{min, max}(s_gen); }

private:
  static std::mt19937 s_gen;
};
}