/*
 * Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "../shader_program_asset.h"
#include "i_asset_loader.h"

struct ShaderProgramLoader final : ew::AssetLoader<ShaderProgramAsset> {
  [[nodiscard]] auto load(ew::IAssetProviderPtr const& provider, const std::string& fn, std::vector<uint8_t> data)
      -> ew::IAssetPtr override;
};