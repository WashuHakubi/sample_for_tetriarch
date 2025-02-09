/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "engine/math.h"
#include "engine/reflection/reflection.h"

#include <cmath>

namespace ewok {
EWOK_REGISTRATION {
  Reflection::class_<Transform>("Transform")
      .field(&Transform::position, "position")
      .field(&Transform::rotation, "rotation")
      .field(&Transform::scale, "scale");
}

Quat fromEuler(Vec3 const& eulerAngles) {
  auto roll = eulerAngles.x;
  auto pitch = eulerAngles.y;
  auto yaw = eulerAngles.z;

  double cr = cos(roll * 0.5);
  double sr = sin(roll * 0.5);
  double cp = cos(pitch * 0.5);
  double sp = sin(pitch * 0.5);
  double cy = cos(yaw * 0.5);
  double sy = sin(yaw * 0.5);

  Quat q;
  q.w = cr * cp * cy + sr * sp * sy;
  q.x = sr * cp * cy - cr * sp * sy;
  q.y = cr * sp * cy + sr * cp * sy;
  q.z = cr * cp * sy - sr * sp * cy;

  return q;
}
} // namespace ewok
