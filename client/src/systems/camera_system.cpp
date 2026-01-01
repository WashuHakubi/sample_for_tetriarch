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
  constexpr glm::vec3 up{0, 1, 0};
  constexpr auto speed = 10.0f;

  camera_.phi += angle_ * dt;

  // A vector pointing from the origin to the camera
  auto const cameraPos = camera_.toCartesian();

  // This should be in a character controller somewhere...
  // Move us in the direction the camera is facing.
  if (left_ || right_ || forward_ || backward_) {
    // get a vector pointing to the origin from the camera (facing direction of the camera)
    // This assumes camera_.r != 0
    auto const facing = glm::vec3{-cameraPos.x, 0, -cameraPos.z};

    // get our right vector, this will be used to compute our strafe momentum
    auto const right = glm::cross(facing, up);

    // compute a movement vector, combining our forward and strafe momentum with our facing direction
    auto const movement = glm::vec3{
        facing.x * ((forward_ ? 1.0f : 0) + (backward_ ? -1.0f : 0)) +
            right.x * ((left_ ? -1.0f : 0) + (right_ ? 1.0f : 0)),
        0,
        facing.z * ((forward_ ? 1.0f : 0) + (backward_ ? -1.0f : 0)) +
            right.z * ((left_ ? -1.0f : 0) + (right_ ? 1.0f : 0))};

    if (movement.x != 0 || movement.z != 0) {
      at_ += glm::normalize(movement) * speed * dt;
    }
  }

  // Rotate the camera around the Y axis
  eye_ = at_ + cameraPos;

  // Update our view to look at `at_` from `eye_`
  auto view = glm::lookAt(eye_, at_, up);
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

    switch (key->scancode) {
      case SCANCODE_W:
        forward_ = key->down;
        break;
      case SCANCODE_S:
        backward_ = key->down;
        break;
      case SCANCODE_A:
        left_ = key->down;
        break;
      case SCANCODE_D:
        right_ = key->down;
        break;
      default:
        break;
    }
  }

  if (auto const wheel = std::get_if<ew::MouseWheelMsg>(&msg)) {
    // adjusts our distance from at_
    camera_.r += wheel->delta * 0.1f;
  }
}
} // namespace ew