/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cmath>

#include <glm/vec3.hpp>
#include <shared/reflection.h>

// We use spherical coordinates for the orbit camera since it's attached to a target.
struct OrbitCamera {
  float r;
  float theta;
  float phi;

  auto toCartesian() const {
    return glm::vec3(r * std::sin(theta) * std::cos(phi), r * std::cos(theta), r * std::sin(theta) * std::sin(phi));
  }
};

EW_REFLECT(OrbitCamera) {
  return std::make_tuple(
      std::make_tuple("r", &OrbitCamera::r),
      std::make_tuple("phi", &OrbitCamera::phi),
      std::make_tuple("theta", &OrbitCamera::theta));
}
