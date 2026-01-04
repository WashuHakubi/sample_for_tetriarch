/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "sample_terrain_system.h"

#include <random>

#include <glm/ext/quaternion_common.hpp>
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
      bgfx::copy(vertices.data(), vertices.size() * sizeof(PosColorVertex)),
      PosColorVertex::layout());
  tibh_ = bgfx::createIndexBuffer(bgfx::copy(indices.data(), indices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);

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
  constexpr float kEpsilon = 0.001f;

  auto const h = x + height_ / 2.0f;
  auto const w = z + width_ / 2.0f;

  auto const ih0 = static_cast<int>(h);
  auto const iw0 = static_cast<int>(w);
  auto const ih1 = static_cast<int>(h) + 1;
  auto const iw1 = static_cast<int>(w) + 1;

  // Out of bounds, so return a default value
  if (ih0 >= height_ || iw0 >= width_) {
    return 0.0f;
  }

  // Check if our extra points are out of bounds or if we're very close to a specific point, return that point in either
  // case.
  if (ih1 >= height_ || iw1 >= width_ || (std::abs(h - ih0) < kEpsilon && std::abs(w - iw0) < kEpsilon)) {
    return heights_[ih0 * width_ + iw0];
  }

  // Get the % between the truncated coordinates and original coordinates.
  auto const frac_h = h - ih0;
  auto const frac_w = w - iw0;

  // Sample the heights at each of the positions, this essentially samples a rectangle
  const auto h0w0 = heights_[ih0 * width_ + iw0];
  const auto h0w1 = heights_[ih0 * width_ + iw1];
  const auto h1w0 = heights_[ih1 * width_ + iw0];
  const auto h1w1 = heights_[ih1 * width_ + iw1];

  // Blend the sampled heights based on the %
  auto const a = glm::mix(h0w0, h0w1, frac_w);
  auto const b = glm::mix(h1w0, h1w1, frac_w);
  return glm::mix(a, b, frac_h);
}
