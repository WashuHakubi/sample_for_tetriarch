/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <bgfx/bgfx.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "../assets/i_asset_provider.h"
#include "../assets/shader_program_asset.h"

struct AxisDebugSystem {
  AxisDebugSystem(ew::IAssetProviderPtr provider, std::shared_ptr<entt::registry> registry);
  ~AxisDebugSystem();

  void render(float dt);

  bgfx::VertexBufferHandle axisVbh_{bgfx::kInvalidHandle};
  bgfx::IndexBufferHandle axisIbh_{bgfx::kInvalidHandle};
  ShaderProgramAsset::Ptr program_;

  ew::IAssetProviderPtr assetProvider_;
  std::shared_ptr<entt::registry> registry_;
};
