/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "sample_terrain_system.h"

#include <random>

#include <glm/ext/quaternion_common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>

#include "../components/pos_color_vertex.h"
#include "../components/terrain_chunk.h"
#include "../components/transform.h"

SampleTerrainSystem::SampleTerrainSystem(ew::AssetProviderPtr provider, entt::registry& registry)
    : assetProvider_(std::move(provider))
    , registry_(&registry) {
  auto terrainChunk = registry_->create();
  auto& chunk = registry_->emplace<TerrainChunk>(terrainChunk);

  auto raw_heightmap = assetProvider_->loadRawAsset("iceland_heightmap.png");

  int channels;

  const auto data = stbi_load_from_memory(
      reinterpret_cast<stbi_uc const*>(raw_heightmap.data()),
      static_cast<int>(raw_heightmap.size()),
      &chunk.width,
      &chunk.height,
      &channels,
      0);

  registry_->emplace<Transform>(
      terrainChunk,
      glm::vec3{-chunk.height / 2.0f, 0, -chunk.width / 2.0f},
      glm::vec3{1.0f},
      glm::quat{});
  chunk.numStrips = chunk.height - 1;
  chunk.numVertsPerStrip = chunk.width * 2;

  std::vector<PosColorVertex> vertices;
  std::vector<uint32_t> indices;

  // Generate our vertices, each row of the height map is a triangle strip
  for (int h = 0; h < chunk.height; ++h) {
    for (int w = 0; w < chunk.width; ++w) {
      constexpr auto yScale = 1.0f / 8.0f;
      constexpr auto yShift = 0.0f;

      const auto texel = data + (w + h * chunk.width) * channels;
      const auto y = *texel;
      const auto color = 0xFF000000 | (y << 16) | (y << 8) | y;

      const auto y_coord = y * yScale + yShift;

      chunk.heights.push_back(y_coord);
      vertices.push_back(
          {{
               static_cast<float>(h),
               y_coord,
               static_cast<float>(w),
           },
           color});
    }
  }

  stbi_image_free(data);

  // generate our triangle strip indices. There are numStrips_ triangle strips, with numVertsPerStrip_ vertices in each
  // strip.
  for (int h = 0; h < chunk.height - 1; ++h) {
    for (int w = 0; w < chunk.width; ++w) {
      for (int k = 0; k < 2; ++k) {
        indices.push_back(w + chunk.width * (h + k));
      }
    }
  }

  chunk.vbh = bgfx::createVertexBuffer(
      bgfx::copy(vertices.data(), vertices.size() * sizeof(PosColorVertex)),
      PosColorVertex::layout());
  chunk.ibh =
      bgfx::createIndexBuffer(bgfx::copy(indices.data(), indices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);

  program_ = assetProvider_->load<ShaderProgram>("cube.json");
}

SampleTerrainSystem::~SampleTerrainSystem() {
  registry_->view<TerrainChunk>().each([](auto ent, auto& chunk) {
    bgfx::destroy(chunk.vbh);
    bgfx::destroy(chunk.ibh);
  });
}

void SampleTerrainSystem::render(float dt) {
  constexpr uint64_t kState = 0 | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
      BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | BGFX_STATE_CULL_CW | BGFX_STATE_PT_TRISTRIP;

  for (auto&& [ent, chunk, transform] : registry_->view<TerrainChunk, Transform>().each()) {
    // Render each strip
    for (int i = 0; i < chunk.numStrips; ++i) {
      auto mat = glm::mat4_cast(transform.rotation);
      mat = glm::scale(mat, transform.scale);
      mat[3][0] = transform.position.x;
      mat[3][1] = transform.position.y;
      mat[3][2] = transform.position.z;
      bgfx::setTransform(glm::value_ptr(mat));
      bgfx::setVertexBuffer(0, chunk.vbh);
      bgfx::setIndexBuffer(chunk.ibh, i * chunk.numVertsPerStrip, chunk.numVertsPerStrip);
      bgfx::setState(kState);
      bgfx::submit(0, program_->programHandle);
    }
  }
}

float SampleTerrainSystem::sample(float x, float z) const {
  constexpr float kEpsilon = 0.001f;

  for (auto&& [ent, chunk, transform] : registry_->view<TerrainChunk, Transform>().each()) {
    auto const h = x - transform.position.x;
    auto const w = z - transform.position.z;

    auto const ih0 = static_cast<int>(h);
    auto const iw0 = static_cast<int>(w);
    auto const ih1 = static_cast<int>(h) + 1;
    auto const iw1 = static_cast<int>(w) + 1;

    // Out of bounds, so return a default value
    if (ih0 >= chunk.height || iw0 >= chunk.width) {
      return 0.0f;
    }

    // Check if our extra points are out of bounds or if we're very close to a specific point, return that point in
    // either case.
    if (ih1 >= chunk.height || iw1 >= chunk.width || (std::abs(h - ih0) < kEpsilon && std::abs(w - iw0) < kEpsilon)) {
      return chunk.heights[ih0 * chunk.width + iw0];
    }

    // Get the % between the truncated coordinates and original coordinates.
    auto const frac_h = h - ih0;
    auto const frac_w = w - iw0;

    // Sample the heights at each of the positions, this essentially samples a rectangle
    const auto h0w0 = chunk.heights[ih0 * chunk.width + iw0];
    const auto h0w1 = chunk.heights[ih0 * chunk.width + iw1];
    const auto h1w0 = chunk.heights[ih1 * chunk.width + iw0];
    const auto h1w1 = chunk.heights[ih1 * chunk.width + iw1];

    // Blend the sampled heights based on the %
    auto const a = glm::mix(h0w0, h0w1, frac_w);
    auto const b = glm::mix(h1w0, h1w1, frac_w);
    return glm::mix(a, b, frac_h);
  }

  return 0.0f;
}
