/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "debug_cubes_rendering_system.h"

#include <bgfx/bgfx.h>
#include <glm/ext.hpp>

struct PosColorVertex {
  glm::vec3 pos;
  uint32_t color;

  static auto& layout() {
    static bool s_init{false};
    static bgfx::VertexLayout s_layout;

    if (!s_init) {
      s_layout.begin()
          .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
          .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
          .end();
      s_init = true;
    }

    return s_layout;
  }
};

DebugCubesRenderingSystem::~DebugCubesRenderingSystem() {
  if (bgfx::isValid(vbh_)) {
    bgfx::destroy(vbh_);
  }
  if (bgfx::isValid(ibh_)) {
    bgfx::destroy(ibh_);
  }
}

void DebugCubesRenderingSystem::render(float dt) {
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

  accum_ += dt;

  // Submit 11x11 cubes.
  for (uint32_t yy = 0; yy < 11; ++yy) {
    for (uint32_t xx = 0; xx < 11; ++xx) {
      glm::mat4x4 mat = glm::identity<glm::mat4x4>();
      mat = glm::rotate(mat, accum_ + xx * 0.21f, glm::vec3(1, 0, 0));
      mat = glm::rotate(mat, accum_ + yy * 0.37f, glm::vec3(0, 1, 0));
      mat[3][0] = -15.0f + xx * 3.0f;
      mat[3][1] = -15.0f + yy * 3.0f;
      mat[3][2] = 0.0f;

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
}