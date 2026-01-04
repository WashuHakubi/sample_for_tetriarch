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

#include "../components/pos_color_vertex.h"

namespace ew {

constexpr float kGravity = -9.8f;

OrbitCameraSystem::OrbitCameraSystem(
    ApplicationPtr app,
    std::shared_ptr<entt::registry> registry,
    std::shared_ptr<SampleTerrainSystem> terrain)
    : app_(std::move(app))
    , registry_(std::move(registry))
    , terrain_(std::move(terrain)) {
  // The entity our camera is following.
  targetEntity_ = registry_->create();

  registry_->emplace<Transform>(
      targetEntity_,
      glm::vec3{0, terrain_->sample(0, 0), 0},
      glm::vec3{3.0f},
      glm::angleAxis(glm::radians(0.0f), glm::vec3{0, 1, 0}));
  registry_->emplace<CubeDebug>(targetEntity_);
  registry_->emplace<OrbitCamera>(targetEntity_, 60.0f, glm::radians(-45.0f), glm::radians(0.0f));
}

void OrbitCameraSystem::update(float dt) {
  auto [transform, camera] = registry_->get<Transform, OrbitCamera>(targetEntity_);

  camera.r = zoom_;
  camera.phi += (angle_ + (inputStates_[UnlockAngle] ? singleFrameAngle_ : 0)) * dt;
  // Ensure phi remains between [0, 2pi]. This isn't necessary, but it makes debugging easier.
  if (camera.phi > glm::two_pi<float>()) {
    camera.phi -= glm::two_pi<float>();
  } else if (camera.phi < 0.0f) {
    camera.phi += glm::two_pi<float>();
  }
  singleFrameAngle_ = 0;

  // A vector pointing from the origin to the camera
  auto const cameraPos = camera.toCartesian();

  // This should be in a character controller somewhere...
  // get a vector pointing to the origin from the camera (the direction the camera is facing)
  // This assumes camera_.r != 0.
  // This does not need to normalize this as we do that when the movement vector is applied to the position.
  auto const facing = glm::vec3{-cameraPos.x, 0, -cameraPos.z};

  // get the right vector, this will be used to compute our strafe momentum
  auto const right = glm::cross(facing, kUp);

  // compute a movement vector, combining our forward and strafe momentum with our facing direction
  auto const movement = glm::vec3{
      facing.x * ((inputStates_[Forward] ? 1.0f : 0) + (inputStates_[Backward] ? -1.0f : 0)) +
          right.x * ((inputStates_[Left] ? 1.0f : 0) + (inputStates_[Right] ? -1.0f : 0)),
      0,
      facing.z * ((inputStates_[Forward] ? 1.0f : 0) + (inputStates_[Backward] ? -1.0f : 0)) +
          right.z * ((inputStates_[Left] ? 1.0f : 0) + (inputStates_[Right] ? -1.0f : 0))};

  // Only update if we actually have movement
  if (movement.x != 0 || movement.z != 0) {
    auto const speed = inputStates_[Sprint] ? 30.0f : 10.0f;

    // Update our character to face in the direction of our movement
    transform.rotation = glm::angleAxis(-camera.phi, glm::vec3{0, 1, 0});
    transform.position += glm::normalize(movement) * speed * dt;
  }

  auto terrainHeight = terrain_->sample(transform.position.x, transform.position.z);

  // If we're on the ground and the jump button is pressed then jump
  if (transform.position.y <= terrainHeight && inputStates_[Jump]) {
    yVelocity_ = 6.0f;
  }

  // Apply the jump velocity
  transform.position.y += yVelocity_ * dt;
  yVelocity_ += kGravity * dt;

  // If we've ended up at a position lower than the terrain, adjust ourselves to be on the terrain and set the
  // velocity to 0.
  if (transform.position.y <= terrainHeight) {
    transform.position.y = terrainHeight;
    yVelocity_ = 0;
  }
}

void OrbitCameraSystem::render(float const dt) {
  auto [transform, camera] = registry_->get<Transform, OrbitCamera>(targetEntity_);

  auto const cameraPos = camera.toCartesian();

  // Get the camera position relative to the target
  auto const eye = transform.position + cameraPos;

  // Update our view to look at `target` from `eye`
  auto const view = glm::lookAt(eye, transform.position, kUp);
  bgfx::setViewTransform(0, glm::value_ptr(view), glm::value_ptr(proj_));

  // Print some debug information
  bgfx::dbgTextPrintf(
      0,
      2,
      0x0f,
      "Camera position: (%.2f, %.2f, %.2f), rotation: %.2f",
      eye.x,
      eye.y,
      eye.z,
      glm::degrees(camera.phi));
  bgfx::dbgTextPrintf(
      0, // x
      3, // y
      0x0f,
      "Target position: (%.2f, %.2f, %.2f)",
      transform.position.x,
      transform.position.y,
      transform.position.z);
}

void OrbitCameraSystem::handleMessage(GameThreadMsg const& msg) {
  // Recompute the aspect ratio and projection matrix on resize
  if (auto const resize = std::get_if<ResizeMsg>(&msg)) {
    auto aspectRatio = static_cast<float>(resize->width) / static_cast<float>(resize->height);
    if (bgfx::getCaps()->homogeneousDepth) {
      // Depth goes from -1,1 (OpenGL)
      proj_ = glm::perspectiveNO(glm::radians(60.0f), aspectRatio, 0.1f, 1000.0f);
    } else {
      // Depth goes from 0,1
      proj_ = glm::perspectiveZO(glm::radians(60.0f), aspectRatio, 0.1f, 1000.0f);
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
        inputStates_[Forward] = key->down;
        break;
      case SCANCODE_S:
        inputStates_[Backward] = key->down;
        break;
      case SCANCODE_A:
        inputStates_[Left] = key->down;
        break;
      case SCANCODE_D:
        inputStates_[Right] = key->down;
        break;
      case SCANCODE_LSHIFT:
        inputStates_[Sprint] = key->down;
        break;
      case SCANCODE_SPACE:
        inputStates_[Jump] = key->down;
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
      inputStates_[UnlockAngle] = click->down;
    }
  }
  // Zoom in and out based on the scroll-wheel direction.
  else if (auto const wheel = std::get_if<MouseWheelMsg>(&msg)) {
    // adjusts our distance from the target, ensure we are between our min and max zoom level (r should never be 0)
    zoom_ = std::clamp(zoom_ + wheel->delta * 0.1f, 1.0f, 100.0f);
  }
}
} // namespace ew