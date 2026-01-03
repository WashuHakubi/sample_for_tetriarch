/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "sample_terrain_system.h"

#include <random>

SampleTerrainSystem::SampleTerrainSystem(ew::AssetProviderPtr provider, entt::registry& registry)
    : assetProvider_(std::move(provider))
    , registry_(&registry) {
  const int width = 256;
  const int height = 256;

  numStrips_ = height - 1;
  numVertsPerStrip_ = width * 2;

  // Generate a random terrain sample
  std::mt19937 rng(0);
  std::uniform_real_distribution dist(-1.0f, 1.0f);

  // Generate our vertices, each row of the height map is a triangle strip
  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      auto y = dist(rng);
      auto color = static_cast<unsigned>(0xFF * ((y + 1.0f) / 2.0f));
      color = 0xFF000000 | (color << 16) | (color << 8) | color;
      vertices_.push_back(
          {{
               -height / 2.0f + static_cast<float>(h),
               y,
               -width / 2.0f + static_cast<float>(w),
           },
           color});
    }
  }

  // generate our triangle strip indices. There are numStrips_ triangle strips, with numVertsPerStrip_ vertices in each
  // strip.
  for (int h = 0; h < height - 1; ++h) {
    for (int w = 0; w < width; ++w) {
      for (int k = 0; k < 2; ++k) {
        indices_.push_back(w + width * (h + k));
      }
    }
  }

  tvbh_ = bgfx::createVertexBuffer(
      bgfx::makeRef(vertices_.data(), vertices_.size() * sizeof(PosColorVertex)),
      PosColorVertex::layout());
  tibh_ = bgfx::createIndexBuffer(bgfx::makeRef(indices_.data(), indices_.size() * sizeof(uint16_t)));

  program_ = assetProvider_->load<ShaderProgram>("cube.json");
}

void SampleTerrainSystem::render(float dt) {
  constexpr uint64_t kState = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
      BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | BGFX_STATE_CULL_CW | BGFX_STATE_PT_TRISTRIP;

  // Render each strip
  for (int i = 0; i < numStrips_; ++i) {
    bgfx::setVertexBuffer(0, tvbh_);
    bgfx::setIndexBuffer(tibh_, i * numVertsPerStrip_, numVertsPerStrip_);
    bgfx::setState(kState);
    bgfx::submit(0, program_->programHandle);
  }
}
