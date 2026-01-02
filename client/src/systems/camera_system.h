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
struct OrbitCamera {
  float r;
  float theta;
  float phi;

  auto toCartesian() const {
    return glm::vec3(r * std::sin(theta) * std::cos(phi), r * std::cos(theta), r * std::sin(theta) * std::sin(phi));
  }
};

struct CameraSystem {
  explicit CameraSystem(entt::registry& registry, ApplicationPtr app);

  void render(float dt);

  void handleMessage(ew::GameThreadMsg const& msg);

  int width_{0};
  int height_{0};
  float aspectRatio_;

  float angle_{0.0f};
  bool unlockAngle_{false};
  float singleFrameAngle_{0.0f};
  float mouseSensitivity_{0.5f};

  bool forward_{false};
  bool backward_{false};
  bool left_{false};
  bool right_{false};

  glm::vec3 eye_{0, 0, -35};
  glm::mat4x4 proj_;

  entt::registry* registry_;
  entt::entity targetEntity_;

  ApplicationPtr app_;
  OrbitCamera camera_{-35.0f, glm::radians(120.0f), glm::radians(90.0f)};
};
} // namespace ew
