/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <glm/ext.hpp>
#include <glm/vec3.hpp>

namespace wut {
struct Transform {
  glm::vec3 position;
  glm::vec3 scale;
  glm::quat rotation;
};
} // namespace wut
