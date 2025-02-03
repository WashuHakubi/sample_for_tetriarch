/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"

namespace ewok {
struct Vec3 {
  float x;
  float y;
  float z;
};

struct Quat {
  float x;
  float y;
  float z;
  float w;
};

constexpr Vec3 asEuler(float roll, float pitch, float yaw) {
  return Vec3{roll, pitch, yaw};
}

// Assumes x = roll, y = pitch, z = yaw
Quat fromEuler(Vec3 const& eulerAngles);

struct Transform {
  Vec3 position;
  Vec3 scale;
  Quat rotation;
};
} // namespace ewok
