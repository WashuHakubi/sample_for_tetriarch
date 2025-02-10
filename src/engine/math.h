/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"

namespace ewok {
template <class T, size_t N>
struct VertexN {
  T v[N];
};

using Vec2 = VertexN<float, 2>;
using Vec3 = VertexN<float, 3>;
using Vec4 = VertexN<float, 4>;

using Int8Vec2 = VertexN<int8_t, 2>;
using Int8Vec3 = VertexN<int8_t, 3>;
using Int8Vec4 = VertexN<int8_t, 4>;

using UInt8Vec2 = VertexN<uint8_t, 2>;
using UInt8Vec3 = VertexN<uint8_t, 3>;
using UInt8Vec4 = VertexN<uint8_t, 4>;

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

Vec3 toEuler(Quat const& quat);

struct Transform {
  Vec3 position{};
  Vec3 scale{};
  Quat rotation{};
};
} // namespace ewok
