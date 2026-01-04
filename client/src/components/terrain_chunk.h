/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <vector>

#include <bgfx/bgfx.h>

struct TerrainChunk {
  int width;
  int height;
  int numStrips;
  int numVertsPerStrip;

  std::vector<float> heights;
  bgfx::VertexBufferHandle vbh;
  bgfx::IndexBufferHandle ibh;
};