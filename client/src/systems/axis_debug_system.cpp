/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "axis_debug_system.h"
#include "../components/debug_components.h"
#include "../components/pos_color_vertex.h"
#include "../components/transform.h"

#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

AxisDebugSystem::AxisDebugSystem(ew::AssetProviderPtr provider, entt::registry& registry)
    : assetProvider_(std::move(provider))
    , registry_(&registry) {
  // Origin axis
  auto const axis = registry_->create();
  registry_->emplace<Transform>(axis, glm::vec3{0}, glm::vec3{10.0f});
  registry_->emplace<AxisDebug>(axis);
}

AxisDebugSystem::~AxisDebugSystem() {
  if (bgfx::isValid(axisVbh_)) {
    bgfx::destroy(axisVbh_);
  }
  if (bgfx::isValid(axisIbh_)) {
    bgfx::destroy(axisIbh_);
  }
}
void AxisDebugSystem::render(float dt) {
  static constexpr PosColorVertex axisVertices[] = {
      {{0.0f, 0.0f, 0.0f}, 0xffffffff},
      {{1.0f, 0.0f, 0.0f}, 0xff0000ff},
      {{0.0f, 1.0f, 0.0f}, 0xff00ff00},
      {{0.0f, 0.0f, 1.0f}, 0xffff0000},
  };

  static constexpr uint16_t axisLineList[] = {
      0,
      1, // X
      0,
      2, // Y
      0,
      3, // Z
  };

  if (!program_) {
    program_ = assetProvider_->load<ShaderProgram>("cube.json");
    return;
  }

  if (!bgfx::isValid(axisVbh_)) {
    axisVbh_ = bgfx::createVertexBuffer(
        // Static data can be passed with bgfx::makeRef
        bgfx::makeRef(axisVertices, sizeof(axisVertices)),
        PosColorVertex::layout());
    bgfx::setName(axisVbh_, "axis debug");
  }

  if (!bgfx::isValid(axisIbh_)) {
    // Create a static index buffer for triangle list rendering.
    axisIbh_ = bgfx::createIndexBuffer(
        // Static data can be passed with bgfx::makeRef
        bgfx::makeRef(axisLineList, sizeof(axisLineList)));
    bgfx::setName(axisIbh_, "axis debug");
  }

  constexpr uint64_t state = 0 | //
      BGFX_STATE_WRITE_MASK | // Write color buffer, alpha and z
      BGFX_STATE_DEPTH_TEST_LESS | //
      BGFX_STATE_MSAA | //
      BGFX_STATE_CULL_CW | //
      BGFX_STATE_PT_LINES;

  for (auto [ent, axis] : registry_->view<AxisDebug, Transform>().each()) {
    auto mat = glm::identity<glm::mat4>();
    mat = glm::translate(mat, axis.position);
    mat = glm::scale(mat, axis.scale);
    mat = mat * glm::mat4_cast(axis.rotation);

    bgfx::setTransform(glm::value_ptr(mat));
    bgfx::setVertexBuffer(0, axisVbh_);
    bgfx::setIndexBuffer(axisIbh_);
    bgfx::setState(state);
    bgfx::submit(0, program_->programHandle);
  }
}
