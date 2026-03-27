/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <glm/ext.hpp>
#include <glm/vec3.hpp>
#include <wut/serialization.h>

namespace wut {
struct Transform {
  glm::vec3 position;
  glm::vec3 scale;
  glm::quat rotation;

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_tuple("position", &Transform::position),
        std::make_tuple("scale", &Transform::scale),
        std::make_tuple("rotation", &Transform::rotation));
  }
};
} // namespace wut
