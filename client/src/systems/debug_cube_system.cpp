/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "debug_cube_system.h"
#include "../components/debug_components.h"
#include "../components/pos_color_vertex.h"
#include "../components/transform.h"

#include <bgfx/bgfx.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static constexpr PosColorVertex cubeVertices[] = {
    {{-1.0f, 1.0f, 1.0f}, 0xff000000},
    {{1.0f, 1.0f, 1.0f}, 0xff0000ff},
    {{-1.0f, -1.0f, 1.0f}, 0xff00ff00},
    {{1.0f, -1.0f, 1.0f}, 0xff00ffff},
    {{-1.0f, 1.0f, -1.0f}, 0xffff0000},
    {{1.0f, 1.0f, -1.0f}, 0xffff00ff},
    {{-1.0f, -1.0f, -1.0f}, 0xffffff00},
    {{1.0f, -1.0f, -1.0f}, 0xffffffff},
};

static constexpr uint16_t cubeTriList[] = {
    0, 1, 2, // 0
    1, 3, 2, //
    4, 6, 5, // 2
    5, 6, 7, //
    0, 2, 4, // 4
    4, 2, 6, //
    1, 5, 3, // 6
    5, 7, 3, //
    0, 4, 1, // 8
    4, 5, 1, //
    2, 3, 6, // 10
    6, 3, 7,
};

DebugCubeSystem::DebugCubeSystem(ew::IAssetProviderPtr provider, std::shared_ptr<entt::registry> registry)
    : assetProvider_(std::move(provider))
    , registry_(std::move(registry)) {
  vbh_ = bgfx::createVertexBuffer(
      // Static data can be passed with bgfx::makeRef
      bgfx::makeRef(cubeVertices, sizeof(cubeVertices)),
      PosColorVertex::layout());

  // Create a static index buffer for triangle list rendering.
  ibh_ = bgfx::createIndexBuffer(
      // Static data can be passed with bgfx::makeRef
      bgfx::makeRef(cubeTriList, sizeof(cubeTriList)));

  program_ = assetProvider_->load<ShaderProgramAsset>("cube.json");
}

DebugCubeSystem::~DebugCubeSystem() {
  if (bgfx::isValid(vbh_)) {
    bgfx::destroy(vbh_);
  }
  if (bgfx::isValid(ibh_)) {
    bgfx::destroy(ibh_);
  }
}

void DebugCubeSystem::render(float dt) {
  constexpr uint64_t state = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
      BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | BGFX_STATE_CULL_CW;

  if (!program_) {
    program_ = assetProvider_->load<ShaderProgramAsset>("cube.json");
    return;
  }

  if (!bgfx::isValid(vbh_)) {
  }

  if (!bgfx::isValid(ibh_)) {
  }

  for (auto&& [ent, transform] : registry_->view<CubeDebug, Transform>().each()) {
    auto mat = glm::identity<glm::mat4x4>();
    mat = glm::translate(mat, transform.position);
    mat = glm::scale(mat, transform.scale);
    mat = mat * glm::mat4_cast(transform.rotation);

    bgfx::setTransform(glm::value_ptr(mat));

    // Set vertex and index buffer.
    bgfx::setVertexBuffer(0, vbh_);
    bgfx::setIndexBuffer(ibh_);
    // Set render states.
    bgfx::setState(state);

    // Submit primitive for rendering to view 0.
    bgfx::submit(0, program_->programHandle);
  }
}