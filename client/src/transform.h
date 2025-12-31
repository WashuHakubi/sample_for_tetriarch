/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once
#include <bx/math.h>

namespace ew {
struct Transform {
  bx::Vec3 translation{0.0f, 0.0f, 0.0f};
  bx::Vec3 rotation{0.0f, 0.0f, 0.0f};
  bx::Vec3 scale{1.0f, 1.0f, 1.0f};
};
} // namespace ew
