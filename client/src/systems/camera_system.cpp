/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "camera_system.h"

#include <bgfx/bgfx.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_projection.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.inl>

namespace ew {
CameraSystem::CameraSystem(entt::registry& registry) : aspectRatio_(0), registry_(&registry) {
  homogeneousDepth_ = bgfx::getCaps()->homogeneousDepth;
}

void CameraSystem::render(float dt) {
  // Rotate the camera around the Y axis
  camera_.phi += angle_ * dt;
  eye_ = camera_.toCartesian();

  // Update our view to look at `at_` from `eye_`
  auto view = glm::lookAt(eye_, at_, {0, 1, 0});
  auto proj = glm::perspective(glm::radians(60.0f), aspectRatio_, 0.1f, 1000.0f);
  bgfx::setViewTransform(0, glm::value_ptr(view), glm::value_ptr(proj));
}

void CameraSystem::handleMessage(ew::Msg const& msg) {
  if (auto resize = std::get_if<ew::ResizeMsg>(&msg)) {
    width_ = resize->width;
    height_ = resize->height;
    aspectRatio_ = static_cast<float>(width_) / static_cast<float>(height_);
  }

  if (auto key = std::get_if<ew::KeyMsg>(&msg)) {
    if (key->scancode == ew::Scancode::SCANCODE_Q || key->scancode == ew::Scancode::SCANCODE_E) {
      angle_ = key->down ? key->scancode == ew::Scancode::SCANCODE_Q ? 1.0f : -1.0f : 0.0f;
    }
  }

  if (auto const wheel = std::get_if<ew::MouseWheelMsg>(&msg)) {
    // adjusts our distance from at_
    camera_.r += wheel->delta * 0.1f;
  }
}
} // namespace ew