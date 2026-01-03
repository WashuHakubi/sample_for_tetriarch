/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "sample_terrain_system.h"

#include <random>
#include <stb/stb_image.h>

#include "../../../ext/stb/include/stb/stb_image.h"

SampleTerrainSystem::SampleTerrainSystem(ew::AssetProviderPtr provider, entt::registry& registry)
    : assetProvider_(std::move(provider))
    , registry_(&registry) {
  auto raw_heightmap = assetProvider_->loadRawAsset("iceland_heightmap.png");

  int channels;

  const auto data = stbi_load_from_memory(
      reinterpret_cast<stbi_uc const*>(raw_heightmap.data()),
      raw_heightmap.size(),
      &width_,
      &height_,
      &channels,
      0);

  numStrips_ = height_ - 1;
  numVertsPerStrip_ = width_ * 2;

  std::vector<PosColorVertex> vertices;
  std::vector<uint32_t> indices;

  // Generate our vertices, each row of the height map is a triangle strip
  for (int h = 0; h < height_; ++h) {
    for (int w = 0; w < width_; ++w) {
      constexpr auto yScale = 1.0f / 8.0f;
      constexpr auto yShift = 0.0f;

      const auto texel = data + (w + h * width_) * channels;
      const auto y = *texel;
      const auto color = 0xFF000000 | (y << 16) | (y << 8) | y;

      const auto y_coord = y * yScale + yShift;

      heights_.push_back(y_coord);
      vertices.push_back(
          {{
               -height_ / 2.0f + static_cast<float>(h),
               y_coord,
               -width_ / 2.0f + static_cast<float>(w),
           },
           color});
    }
  }

  stbi_image_free(data);

  // generate our triangle strip indices. There are numStrips_ triangle strips, with numVertsPerStrip_ vertices in each
  // strip.
  for (int h = 0; h < height_ - 1; ++h) {
    for (int w = 0; w < width_; ++w) {
      for (int k = 0; k < 2; ++k) {
        indices.push_back(w + width_ * (h + k));
      }
    }
  }

  tvbh_ = bgfx::createVertexBuffer(
      bgfx::makeRef(vertices.data(), vertices.size() * sizeof(PosColorVertex)),
      PosColorVertex::layout());
  tibh_ =
      bgfx::createIndexBuffer(bgfx::makeRef(indices.data(), indices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);

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

float SampleTerrainSystem::sample(float x, float z) const {
  auto const h = x + height_ / 2.0f;
  auto const w = z + width_ / 2.0f;
  auto const idx = static_cast<size_t>(h) * width_ + static_cast<size_t>(w);
  return idx >= heights_.size() ? 0.0f : heights_[idx];
}
