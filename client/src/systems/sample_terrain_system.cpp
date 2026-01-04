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
  auto const terrainChunk = registry_->create();
  auto& [width, height, numStrips, numVertsPerStrip, heights, vbh, ibh] =
      registry_->emplace<TerrainChunk>(terrainChunk);

  auto const raw_heightmap = assetProvider_->loadRawAsset("iceland_heightmap.png");

  int channels;

  auto const data = stbi_load_from_memory(
      reinterpret_cast<stbi_uc const*>(raw_heightmap.data()),
      static_cast<int>(raw_heightmap.size()),
      &width,
      &height,
      &channels,
      0);

  registry_->emplace<Transform>(
      terrainChunk,
      // center the chunk over the origin
      glm::vec3{-static_cast<float>(height) / 2.0f, 0, -static_cast<float>(width) / 2.0f},
      glm::vec3{1.0f},
      glm::quat{});
  numStrips = height - 1;
  numVertsPerStrip = width * 2;

  std::vector<PosColorVertex> vertices;
  std::vector<uint32_t> indices;

  // Generate our vertices, each row of the height map is a triangle strip
  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      constexpr auto yScale = 1.0f / 8.0f;
      constexpr auto yShift = 0.0f;

      const auto texel = data + (w + h * width) * channels;
      const auto y = *texel;
      const auto color = 0xFF000000 | (y << 16) | (y << 8) | y;

      const auto y_coord = y * yScale + yShift;

      heights.push_back(y_coord);
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

  // generate our triangle strip indices. There are numStrips triangle strips, with numVertsPerStrip vertices in each
  // strip.
  for (int h = 0; h < height - 1; ++h) {
    for (int w = 0; w < width; ++w) {
      for (int k = 0; k < 2; ++k) {
        indices.push_back(w + width * (h + k));
      }
    }
  }

  vbh = bgfx::createVertexBuffer(
      bgfx::copy(vertices.data(), vertices.size() * sizeof(PosColorVertex)),
      PosColorVertex::layout());
  ibh = bgfx::createIndexBuffer(bgfx::copy(indices.data(), indices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);

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
    // translate the coordinates into terrain space.
    auto const h = x - transform.position.x;
    auto const w = z - transform.position.z;

    auto const ih0 = static_cast<int>(h);
    auto const iw0 = static_cast<int>(w);
    auto const ih1 = static_cast<int>(h) + 1;
    auto const iw1 = static_cast<int>(w) + 1;

    // Out of bounds, skip
    if (ih0 >= chunk.height || iw0 >= chunk.width) {
      continue;
    }

    // Check if our extra points are out of bounds or if we're very close to a specific point, return that point in
    // either case.
    if (ih1 >= chunk.height || iw1 >= chunk.width ||
        (std::abs(h - static_cast<float>(ih0)) < kEpsilon && std::abs(w - static_cast<float>(iw0)) < kEpsilon)) {
      return chunk.heights[ih0 * chunk.width + iw0];
    }

    // Get the % between the truncated coordinates and original coordinates.
    auto const frac_h = h - static_cast<float>(ih0);
    auto const frac_w = w - static_cast<float>(iw0);

    // Sample the heights at each of the positions, this essentially samples a rectangle
    auto const h0w0 = chunk.heights[ih0 * chunk.width + iw0];
    auto const h0w1 = chunk.heights[ih0 * chunk.width + iw1];
    auto const h1w0 = chunk.heights[ih1 * chunk.width + iw0];
    auto const h1w1 = chunk.heights[ih1 * chunk.width + iw1];

    // Blend the sampled heights based on the %
    auto const a = glm::mix(h0w0, h0w1, frac_w);
    auto const b = glm::mix(h1w0, h1w1, frac_w);
    return glm::mix(a, b, frac_h);
  }

  // Not found or no terrain chunks
  return 0.0f;
}
