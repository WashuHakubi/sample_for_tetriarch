/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once
#include "../assets/i_asset.h"
#include "../assets/shader_program_asset.h"

struct DebugCubesRenderingSystem {
  explicit DebugCubesRenderingSystem(ew::AssetProviderPtr provider) : assetProvider_(std::move(provider)) {}

  ~DebugCubesRenderingSystem();

  void render(float dt);

  float accum_{0};
  bgfx::VertexBufferHandle vbh_{bgfx::kInvalidHandle};
  bgfx::IndexBufferHandle ibh_{bgfx::kInvalidHandle};
  ShaderProgram::Ptr program_;
  ew::AssetProviderPtr assetProvider_;
};