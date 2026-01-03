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

struct SampleTag {};

DebugCubeSystem::DebugCubeSystem(
    ew::AssetProviderPtr provider,
    entt::registry& registry,
    std::shared_ptr<SampleTerrainSystem> terrain)
    : assetProvider_(std::move(provider))
    , registry_(&registry) {
  for (size_t xx = 0; xx < 11; ++xx) {
    for (size_t zz = 0; zz < 11; ++zz) {
      auto e = registry_->create();
      auto x = static_cast<float>(xx) * 3.0f - 15.0f;
      auto z = static_cast<float>(zz) * 3.0f - 15.0f;
      registry.emplace<CubeDebug>(e);
      registry.emplace<SampleTag>(e);
      registry.emplace<Transform>(
          e,
          glm::vec3{
              x,
              terrain->sample(x, z),
              z,
          },
          glm::vec3{1.0f},
          glm::angleAxis(static_cast<float>(xx) * 0.21f, glm::vec3{1, 0, 0}) *
              glm::angleAxis(static_cast<float>(zz) * 0.37f, glm::vec3{0, 1, 0}));
    }
  }

  program_ = assetProvider_->load<ShaderProgram>("cube.json");
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

  constexpr uint64_t state = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
      BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | BGFX_STATE_CULL_CW;

  if (!program_) {
    program_ = assetProvider_->load<ShaderProgram>("cube.json");
    return;
  }

  if (!bgfx::isValid(vbh_)) {
    vbh_ = bgfx::createVertexBuffer(
        // Static data can be passed with bgfx::makeRef
        bgfx::makeRef(cubeVertices, sizeof(cubeVertices)),
        PosColorVertex::layout());
  }

  if (!bgfx::isValid(ibh_)) {
    // Create a static index buffer for triangle list rendering.
    ibh_ = bgfx::createIndexBuffer(
        // Static data can be passed with bgfx::makeRef
        bgfx::makeRef(cubeTriList, sizeof(cubeTriList)));
  }

  for (auto&& [ent, transform] : registry_->view<CubeDebug, Transform, SampleTag>().each()) {
    transform.rotation *= glm::angleAxis(dt, glm::vec3(1, 1, 0));
  }

  for (auto&& [ent, transform] : registry_->view<CubeDebug, Transform>().each()) {
    auto mat = glm::mat4_cast(transform.rotation);
    mat = glm::scale(mat, transform.scale);
    mat[3][0] = transform.position.x;
    mat[3][1] = transform.position.y;
    mat[3][2] = transform.position.z;

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