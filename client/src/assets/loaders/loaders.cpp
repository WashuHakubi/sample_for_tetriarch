/*
 * Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "../loaders.h"
#include "asset_provider.h"

#include "heightmap_asset_loader.h"
#include "shader_program_loader.h"

namespace ew {
IAssetProviderPtr createAssetProvider(
    IFileProviderPtr fileProvider,
    std::shared_ptr<coro::io_scheduler> assetScheduler) {
  auto provider = std::make_shared<AssetProvider>(std::move(fileProvider), std::move(assetScheduler));

  // All asset loaders should be included here and registered
  // This helps to ensure loaders aren't leaking into other pieces of code
  provider->registerAssetLoader(std::make_shared<HeightmapAssetLoader>());
  provider->registerAssetLoader(std::make_shared<ShaderProgramLoader>());

  return provider;
}
} // namespace ew