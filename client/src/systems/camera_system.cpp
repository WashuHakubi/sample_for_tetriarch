/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "camera_system.h"
#include "debug_cubes_rendering_system.h"

#include <bgfx/bgfx.h>
#include <ng-log/logging.h>

namespace ew {

CameraSystem::CameraSystem(entt::registry& registry, ApplicationPtr app)
    : aspectRatio_(0)
    , registry_(&registry)
    , app_(std::move(app)) {
  targetEntity_ = registry_->create();
  registry_->emplace<AxisDebugEntity>(
      targetEntity_,
      glm::vec3{},
      glm::quat{glm::angleAxis(-camera_.phi, glm::vec3{0, 1, 0})},
      10.0f);
}

void CameraSystem::render(float dt) {
  constexpr glm::vec3 up{0, 1, 0};
  constexpr auto speed = 10.0f;

  auto& ade = registry_->get<AxisDebugEntity>(targetEntity_);

  camera_.phi += (angle_ + (unlockAngle_ ? singleFrameAngle_ : 0)) * dt;
  singleFrameAngle_ = 0;

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
    auto const movement = glm::normalize(
        glm::vec3{
            facing.x * ((forward_ ? 1.0f : 0) + (backward_ ? -1.0f : 0)) +
                right.x * ((left_ ? 1.0f : 0) + (right_ ? -1.0f : 0)),
            0,
            facing.z * ((forward_ ? 1.0f : 0) + (backward_ ? -1.0f : 0)) +
                right.z * ((left_ ? 1.0f : 0) + (right_ ? -1.0f : 0))});

    if (movement.x != 0 || movement.z != 0) {
      // Update our character to face in the direction of our movement
      ade.rotation = glm::angleAxis(-camera_.phi, glm::vec3{0, 1, 0});
      ade.position += movement * speed * dt;
    }
  }

  // Rotate the camera around the Y axis
  eye_ = ade.position + cameraPos;

  bgfx::dbgTextPrintf(0, 2, 0x0f, "Camera position: (%.2f, %.2f, %.2f)", eye_.x, eye_.y, eye_.z);
  bgfx::dbgTextPrintf(
      0, // x
      3, // y
      0x0f,
      "Target position: (%.2f, %.2f, %.2f)",
      ade.position.x,
      ade.position.y,
      ade.position.z);
  bgfx::dbgTextPrintf(0, 4, 0x0f, "Camera rotation: %.2f", glm::degrees(camera_.phi));

  // Update our view to look at `at_` from `eye_`
  auto view = glm::lookAt(eye_, ade.position, up);
  bgfx::setViewTransform(0, glm::value_ptr(view), glm::value_ptr(proj_));
}

void CameraSystem::handleMessage(ew::GameThreadMsg const& msg) {
  if (auto resize = std::get_if<ew::ResizeMsg>(&msg)) {
    width_ = resize->width;
    height_ = resize->height;
    aspectRatio_ = static_cast<float>(width_) / static_cast<float>(height_);
    if (bgfx::getCaps()->homogeneousDepth) {
      // Depth goes from -1,1 (OpenGL)
      proj_ = glm::perspectiveNO(glm::radians(60.0f), aspectRatio_, 0.1f, 1000.0f);
    } else {
      // Depth goes from 0,1
      proj_ = glm::perspectiveZO(glm::radians(60.0f), aspectRatio_, 0.1f, 1000.0f);
    }
  } else if (auto key = std::get_if<ew::KeyMsg>(&msg)) {
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
  } else if (auto motion = std::get_if<ew::MouseMotionMsg>(&msg)) {
    singleFrameAngle_ = motion->relPosition.x * mouseSensitivity_;
  } else if (auto const click = std::get_if<ew::MouseButtonMsg>(&msg)) {
    if (click->button == MouseButton::Right) {
      app_->sendMainThreadMessage(CaptureMouseMsg{click->down});
      unlockAngle_ = click->down;
    }
  } else if (auto const wheel = std::get_if<ew::MouseWheelMsg>(&msg)) {
    // adjusts our distance from at_
    camera_.r += wheel->delta * 0.1f;
  }
}
} // namespace ew