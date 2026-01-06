/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <shared/reflection.h>

struct Transform {
  glm::vec3 position{0};
  glm::vec3 scale{1};
  glm::quat rotation;
};

constexpr glm::vec3 kUp{0, 1, 0};
