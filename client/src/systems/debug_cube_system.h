/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "../assets/i_asset.h"
#include "../assets/shader_program_asset.h"

#include <entt/entt.hpp>

struct DebugCubeSystem {
  explicit DebugCubeSystem(ew::AssetProviderPtr provider, entt::registry& registry);

  ~DebugCubeSystem();

  void render(float dt);

  bgfx::VertexBufferHandle vbh_{bgfx::kInvalidHandle};
  bgfx::IndexBufferHandle ibh_{bgfx::kInvalidHandle};
  ShaderProgram::Ptr program_;
  ew::AssetProviderPtr assetProvider_;
  entt::registry* registry_;
};