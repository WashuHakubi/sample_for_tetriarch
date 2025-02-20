/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#define _USE_MATH_DEFINES
#include "engine/math.h"
#include "engine/reflection/reflection.h"

#include <cmath>

EWOK_REGISTRATION {
  using namespace ewok;

  Register::class_<Transform>("Transform")
      .field(&Transform::position, "position")
      .field(&Transform::rotation, "rotation")
      .field(&Transform::scale, "scale");
}

namespace ewok {
Quat fromEuler(Vec3 const& eulerAngles) {
  auto roll = eulerAngles.v[0];
  auto pitch = eulerAngles.v[1];
  auto yaw = eulerAngles.v[2];

  auto cr = cosf(roll * 0.5f);
  auto sr = sinf(roll * 0.5f);
  auto cp = cosf(pitch * 0.5f);
  auto sp = sinf(pitch * 0.5f);
  auto cy = cosf(yaw * 0.5f);
  auto sy = sinf(yaw * 0.5f);

  Quat q;
  q.w = cr * cp * cy + sr * sp * sy;
  q.x = sr * cp * cy - cr * sp * sy;
  q.y = cr * sp * cy + sr * cp * sy;
  q.z = cr * cp * sy - sr * sp * cy;

  return q;
}

Vec3 toEuler(Quat const& q) {
  const auto xx = q.x;
  const auto yy = q.y;
  const auto zz = q.z;
  const auto ww = q.w;
  const auto xsq = xx * xx;
  const auto ysq = yy * yy;
  const auto zsq = zz * zz;

  return {
      std::atan2(2.0f * (xx * ww - yy * zz), 1.0f - 2.0f * (xsq + zsq)),
      std::atan2(2.0f * (yy * ww + xx * zz), 1.0f - 2.0f * (ysq + zsq)),
      std::asin(2.0f * (xx * yy + zz * ww)),
  };
}
} // namespace ewok
