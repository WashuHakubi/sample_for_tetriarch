/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cmath>

#include <entt/entt.hpp>

#include "../i_application.h"

namespace ew {
// We use spherical coordinates for the orbit camera since it's attached to a target.
struct OrbitCamera {
  float r;
  float theta;
  float phi;

  auto toCartesian() const {
    return glm::vec3(r * std::sin(theta) * std::cos(phi), r * std::cos(theta), r * std::sin(theta) * std::sin(phi));
  }
};

struct OrbitCameraSystem {
  explicit OrbitCameraSystem(entt::registry& registry, ApplicationPtr app);

  void render(float dt);

  void handleMessage(ew::GameThreadMsg const& msg);

  int width_{0};
  int height_{0};
  float aspectRatio_;

  float angle_{0.0f};
  float zoom_{35.0f};
  bool unlockAngle_{false};
  float singleFrameAngle_{0.0f};
  float mouseSensitivity_{0.5f};

  enum MovementDirection { Forward, Backward, Left, Right };
  std::bitset<sizeof(MovementDirection) * CHAR_BIT> movementDirections_;

  glm::mat4x4 proj_{};

  ApplicationPtr app_;
  entt::registry* registry_;
  entt::entity targetEntity_;
};
} // namespace ew
