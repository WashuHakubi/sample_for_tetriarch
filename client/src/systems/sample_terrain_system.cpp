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
#include <ng-log/logging.h>
#include <stb/stb_image.h>

#include "../assets/heightmap_asset.h"
#include "../components/pos_color_vertex.h"
#include "../components/terrain_chunk.h"
#include "../components/transform.h"

namespace {
auto loadAndCreateSampleTerrain(ew::IAssetProviderPtr provider, std::shared_ptr<entt::registry> registry)
    -> coro::task<> {
  // Add a terrain chunk and get references to the components of the chunk
  TerrainChunk chunk = {};

  // Load image data for our terrain
  auto heightmap = co_await provider->loadAsync<HeightmapAsset>("iceland_heightmap.png");
  if (!heightmap) {
    LOG(ERROR) << "Failed to load iceland_heightmap.png";
    co_return;
  }

  chunk.width = heightmap->width();
  chunk.height = heightmap->height();

  chunk.numStrips = chunk.height - 1;
  chunk.numVertsPerStrip = chunk.width * 2;

  // Allocate a buffer to contain our vertex and data.
  auto terrainVertexData = bgfx::alloc(sizeof(PosColorVertex) * chunk.height * chunk.width);
  auto vertices = reinterpret_cast<PosColorVertex*>(terrainVertexData->data);
  auto index = 0;

  // Generate our vertices, each row of the height map is a triangle strip
  for (int h = 0; h < chunk.height; ++h) {
    for (int w = 0; w < chunk.width; ++w) {
      constexpr auto yScale = 1.0f / 8.0f;
      constexpr auto yShift = 0.0f;

      const auto y = heightmap->sample(w, h);
      const auto color = 0xFF000000 | (y << 16) | (y << 8) | y;

      const auto y_coord = y * yScale + yShift;

      chunk.heights.push_back(y_coord);

      vertices[index++] = {
          {
              static_cast<float>(h),
              y_coord,
              static_cast<float>(w),
          },
          color};
    }
  }

  // Allocate a buffer to contain our indices.
  auto terrainIndexData = bgfx::alloc(sizeof(uint32_t) * chunk.width * chunk.height * 2);
  auto indices = reinterpret_cast<uint32_t*>(terrainIndexData->data);

  // generate our triangle strip indices. There are numStrips triangle strips, with numVertsPerStrip vertices in each
  // strip.
  index = 0;
  for (int h = 0; h < chunk.height - 1; ++h) {
    for (int w = 0; w < chunk.width; ++w) {
      for (int k = 0; k < 2; ++k) {
        indices[index++] = w + chunk.width * (h + k);
      }
    }
  }

  chunk.vbh = bgfx::createVertexBuffer(terrainVertexData, PosColorVertex::layout());
  chunk.ibh = bgfx::createIndexBuffer(terrainIndexData, BGFX_BUFFER_INDEX32);

  // Create our entity and populate its components. We do this at the very end to ensure that all work has been
  // completed.
  auto const terrainChunk = registry->create();
  registry->emplace<Transform>(
      terrainChunk,
      // center the chunk over the origin
      glm::vec3{-static_cast<float>(chunk.height) / 2.0f, 0, -static_cast<float>(chunk.width) / 2.0f},
      glm::vec3{1.0f},
      glm::quat{});
  registry->emplace<TerrainChunk>(terrainChunk, std::move(chunk));
}
} // namespace

SampleTerrainSystem::SampleTerrainSystem(
    std::shared_ptr<coro::io_scheduler> updateScheduler,
    ew::IAssetProviderPtr provider,
    std::shared_ptr<entt::registry> registry)
    : assetProvider_(std::move(provider))
    , registry_(std::move(registry)) {
  updateScheduler->spawn_detached(loadAndCreateSampleTerrain(assetProvider_, registry_));

  program_ = assetProvider_->load<ShaderProgramAsset>("cube.json");
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

  auto view = registry_->view<TerrainChunk const, Transform const>();
  view.refresh();

  for (auto&& [ent, chunk, transform] : view.each()) {
    if (!bgfx::isValid(chunk.ibh) || !bgfx::isValid(chunk.vbh)) {
      continue;
    }

    // Render each strip
    for (int i = 0; i < chunk.numStrips; ++i) {
      auto mat = glm::identity<glm::mat4>();
      mat = glm::translate(mat, transform.position);
      mat = glm::scale(mat, transform.scale);
      mat = mat * glm::mat4_cast(transform.rotation);

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
    // If the terrain starts at position (1024, 1024) and with width and height of 1024x1024 then:
    // - given x,z (512, 512), translating this into terrain space will result in coordinates of (-512, -512), which are
    // clearly not on the terrain.
    // - given x,z (1536,1536), translating into terrain space will result in coordinates of (512,512), which are on the
    // terrain.
    auto const h = x - transform.position.x;
    auto const w = z - transform.position.z;

    if (h < 0 || w < 0) {
      // Terrain coordinates will always be between 0,0 and width,height.
      return 0.0f;
    }

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
