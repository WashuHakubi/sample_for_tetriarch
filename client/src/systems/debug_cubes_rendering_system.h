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
#include <glm/ext.hpp>
#include <glm/glm.hpp>

struct AxisDebugEntity {
  glm::vec3 position;
  glm::quat rotation;
  float scale{2.0f};
};

struct DebugCubesRenderingSystem {
  explicit DebugCubesRenderingSystem(ew::AssetProviderPtr provider, entt::registry& registry)
      : assetProvider_(std::move(provider))
      , registry_(&registry) {}

  ~DebugCubesRenderingSystem();

  void render(float dt);

  float accum_{0};
  bgfx::VertexBufferHandle axisVbh_{bgfx::kInvalidHandle};
  bgfx::IndexBufferHandle axisIbh_{bgfx::kInvalidHandle};
  bgfx::VertexBufferHandle vbh_{bgfx::kInvalidHandle};
  bgfx::IndexBufferHandle ibh_{bgfx::kInvalidHandle};
  ShaderProgram::Ptr program_;
  ew::AssetProviderPtr assetProvider_;
  entt::registry* registry_;
};