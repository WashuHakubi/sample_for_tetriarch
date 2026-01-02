/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cstdint>

#include <bgfx/bgfx.h>
#include <glm/vec3.hpp>

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
