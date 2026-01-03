/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "orbit_camera_system.h"
#include "../components/debug_components.h"
#include "../components/orbit_camera.h"
#include "../components/transform.h"

#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>
#include <ng-log/logging.h>

namespace ew {
OrbitCameraSystem::OrbitCameraSystem(entt::registry& registry, ApplicationPtr app)
    : aspectRatio_(0)
    , app_(std::move(app))
    , registry_(&registry) {
  // Origin axis
  auto axis = registry_->create();
  registry_->emplace<Transform>(axis, glm::vec3{0}, glm::vec3{10.0f});
  registry_->emplace<AxisDebug>(axis);

  // The entity our camera is following.
  targetEntity_ = registry_->create();

  registry_->emplace<Transform>(
      targetEntity_,
      glm::vec3{0},
      glm::vec3{3.0f},
      glm::angleAxis(glm::radians(0.0f), glm::vec3{0, 1, 0}));
  // registry_->emplace<AxisDebug>(targetEntity_);
  registry_->emplace<CubeDebug>(targetEntity_);
  registry_->emplace<OrbitCamera>(targetEntity_, 35.0f, glm::radians(-45.0f), glm::radians(0.0f));
}

void OrbitCameraSystem::render(float const dt) {
  constexpr glm::vec3 up{0, 1, 0};
  constexpr auto speed = 10.0f;

  auto [transform, camera] = registry_->get<Transform, OrbitCamera>(targetEntity_);

  camera.r = zoom_;
  camera.phi += (angle_ + (movementDirections_[UnlockAngle] ? singleFrameAngle_ : 0)) * dt;
  singleFrameAngle_ = 0;

  // A vector pointing from the origin to the camera
  auto const cameraPos = camera.toCartesian();

  // This should be in a character controller somewhere...
  // Move us in the direction the camera is facing.
  if (movementDirections_.any()) {
    // get a vector pointing to the origin from the camera (facing direction of the camera)
    // This assumes camera_.r != 0.
    // This does not need to normalize this as we do that when the movement vector is applied to the position.
    auto const facing = glm::vec3{-cameraPos.x, 0, -cameraPos.z};

    // get the right vector, this will be used to compute our strafe momentum
    auto const right = glm::cross(facing, up);

    // compute a movement vector, combining our forward and strafe momentum with our facing direction
    auto const movement = glm::vec3{
        facing.x * ((movementDirections_[Forward] ? 1.0f : 0) + (movementDirections_[Backward] ? -1.0f : 0)) +
            right.x * ((movementDirections_[Left] ? 1.0f : 0) + (movementDirections_[Right] ? -1.0f : 0)),
        0,
        facing.z * ((movementDirections_[Forward] ? 1.0f : 0) + (movementDirections_[Backward] ? -1.0f : 0)) +
            right.z * ((movementDirections_[Left] ? 1.0f : 0) + (movementDirections_[Right] ? -1.0f : 0))};

    if (movement.x != 0 || movement.z != 0) {
      // Update our character to face in the direction of our movement
      transform.rotation = glm::angleAxis(-camera.phi, glm::vec3{0, 1, 0});
      transform.position += glm::normalize(movement) * speed * dt;
    }
  }

  // Get the camera position relative to the target
  auto eye = transform.position + cameraPos;

  // Update our view to look at `target` from `eye`
  auto view = glm::lookAt(eye, transform.position, up);
  bgfx::setViewTransform(0, glm::value_ptr(view), glm::value_ptr(proj_));

  // Print some debug information
  bgfx::dbgTextPrintf(0, 2, 0x0f, "Camera position: (%.2f, %.2f, %.2f)", eye.x, eye.y, eye.z);
  bgfx::dbgTextPrintf(
      0, // x
      3, // y
      0x0f,
      "Target position: (%.2f, %.2f, %.2f)",
      transform.position.x,
      transform.position.y,
      transform.position.z);
  bgfx::dbgTextPrintf(0, 4, 0x0f, "Camera rotation: %.2f", glm::degrees(camera.phi));
}

void OrbitCameraSystem::handleMessage(GameThreadMsg const& msg) {
  // Recompute the aspect ratio and projection matrix on resize
  if (auto const resize = std::get_if<ResizeMsg>(&msg)) {
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
  }
  // Handle input. This does not belong here, and instead input should be handled by a system specifically designed for
  // that.
  else if (auto const key = std::get_if<KeyMsg>(&msg)) {
    if (key->scancode == SCANCODE_Q || key->scancode == SCANCODE_E) {
      angle_ = key->down ? key->scancode == SCANCODE_Q ? 1.0f : -1.0f : 0.0f;
    }

    switch (key->scancode) {
      case SCANCODE_W:
        movementDirections_[Forward] = key->down;
        break;
      case SCANCODE_S:
        movementDirections_[Backward] = key->down;
        break;
      case SCANCODE_A:
        movementDirections_[Left] = key->down;
        break;
      case SCANCODE_D:
        movementDirections_[Right] = key->down;
        break;
      default:
        break;
    }
  }
  // Rotate the camera by some amount based on the mouse motion. This is only applied if unlockAngle_ is true.
  else if (auto const motion = std::get_if<MouseMotionMsg>(&msg)) {
    singleFrameAngle_ = motion->relPosition.x * mouseSensitivity_;
  }
  // Unlock the camera while the RMB is held down.
  else if (auto const click = std::get_if<MouseButtonMsg>(&msg)) {
    if (click->button == MouseButton::Right) {
      app_->sendMainThreadMessage(CaptureMouseMsg{click->down});
      movementDirections_[UnlockAngle] = click->down;
    }
  }
  // Zoom in and out based on the scroll-wheel direction.
  else if (auto const wheel = std::get_if<MouseWheelMsg>(&msg)) {
    // adjusts our distance from the target, ensure we are between our min and max zoom level (r should never be 0)
    zoom_ = std::clamp(zoom_ + wheel->delta * 0.1f, 1.0f, 50.0f);
  }
}
} // namespace ew