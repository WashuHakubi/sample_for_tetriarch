/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "debug_components.h"
#include "orbit_camera.h"
#include "terrain_chunk.h"
#include "transform.h"

EW_REFLECT(glm::vec3) {
  return std::make_tuple(
      std::make_tuple("x", &glm::vec3::x),
      std::make_tuple("y", &glm::vec3::y),
      std::make_tuple("z", &glm::vec3::z));
}

EW_REFLECT(glm::quat) {
  return std::make_tuple(
      std::make_tuple("x", &glm::quat::x),
      std::make_tuple("y", &glm::quat::y),
      std::make_tuple("z", &glm::quat::z),
      std::make_tuple("w", &glm::quat::w));
}
