/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once
#include <bgfx/bgfx.h>

#include "i_asset.h"

struct ShaderProgram final : ew::Asset<ShaderProgram> {
  ShaderProgram(bgfx::ProgramHandle handle) : programHandle{handle} {}

  ~ShaderProgram() override { bgfx::destroy(programHandle); }

  bgfx::ProgramHandle programHandle{bgfx::kInvalidHandle};
};

struct ShaderProgramLoader final : ew::AssetLoader<ShaderProgram> {
  [[nodiscard]] auto load(ew::AssetProviderPtr const& provider, const std::string& fn, std::string const& data)
      -> ew::IAssetPtr override;
};