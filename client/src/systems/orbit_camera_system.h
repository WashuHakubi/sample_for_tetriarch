/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <bitset>
#include <cmath>

#include <entt/entt.hpp>

#include "../i_application.h"
#include "sample_terrain_system.h"

namespace ew {
struct OrbitCameraSystem {
  explicit OrbitCameraSystem(
      ApplicationPtr app,
      std::shared_ptr<entt::registry> registry,
      std::shared_ptr<SampleTerrainSystem> terrain);

  void update(float dt);

  void render(float dt);

  void handleMessage(ew::GameThreadMsg const& msg);

  float yVelocity_{0.0f};
  float angle_{0.0f};
  float zoom_{35.0f};
  float singleFrameAngle_{0.0f};
  float mouseSensitivity_{0.5f};

  enum InputStates { Forward, Backward, Left, Right, UnlockAngle, Sprint, Jump };
  std::bitset<sizeof(InputStates) * CHAR_BIT> inputStates_;

  glm::mat4x4 proj_{};

  ApplicationPtr app_;
  std::shared_ptr<entt::registry> registry_;
  std::shared_ptr<SampleTerrainSystem> terrain_;
  entt::entity targetEntity_;
};
} // namespace ew
