/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cmath>

#include <bx/math.h>
#include <entt/entity/registry.hpp>

#include "../i_application.h"
#include "../sim_time.h"

namespace ew {
struct OrbitCamera {
  float r;
  float theta;
  float phi;

  auto toCartesian() const {
    return bx::Vec3(r * std::sin(theta) * std::cos(phi), r * std::cos(theta), r * std::sin(theta) * std::sin(phi));
  }
};

struct CameraSystem {
  explicit CameraSystem(entt::registry& registry);

  void render(float dt);

  void handleMessage(ew::Msg const& msg);

  int width_{0};
  int height_{0};
  float aspectRatio_;

  float angle_{0.0f};
  bx::Vec3 at_{0, 0, 0};
  bx::Vec3 eye_{0, 0, -35};
  bool homogeneousDepth_{false};

  entt::registry* registry_;
  OrbitCamera camera_{-35.0f, bx::toRad(120), bx::toRad(-90)};
};
} // namespace ew
