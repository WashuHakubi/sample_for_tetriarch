/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <bgfx/bgfx.h>
#include <entt/entt.hpp>

#include "../assets/i_asset.h"
#include "../assets/shader_program_asset.h"
#include "../components/pos_color_vertex.h"

class SampleTerrainSystem {
 public:
  SampleTerrainSystem(ew::AssetProviderPtr provider, entt::registry& registry);

  void render(float dt);

 private:
  ew::AssetProviderPtr assetProvider_;
  entt::registry* registry_;

  ShaderProgram::Ptr program_;

  bgfx::IndexBufferHandle tibh_{bgfx::kInvalidHandle};
  bgfx::VertexBufferHandle tvbh_{bgfx::kInvalidHandle};
  std::vector<PosColorVertex> vertices_;
  std::vector<uint16_t> indices_;
  int numStrips_{0};
  int numVertsPerStrip_{0};
};
