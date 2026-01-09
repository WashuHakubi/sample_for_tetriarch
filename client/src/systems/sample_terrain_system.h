/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <bgfx/bgfx.h>
#include <entt/entt.hpp>

#include "../assets/i_asset_provider.h"
#include "../assets/shader_program_asset.h"

class SampleTerrainSystem {
 public:
  SampleTerrainSystem(ew::IAssetProviderPtr provider, std::shared_ptr<entt::registry> registry);
  ~SampleTerrainSystem();

  void render(float dt);

  float sample(float x, float z) const;

 private:
  ew::IAssetProviderPtr assetProvider_;
  std::shared_ptr<entt::registry> registry_;
  ShaderProgramAsset::Ptr program_;
};
