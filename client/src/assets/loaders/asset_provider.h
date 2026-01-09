/*
 * Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "../i_asset_provider.h"
#include "i_asset_loader.h"
#include "i_file_provider.h"

namespace ew {
struct AssetProvider : IAssetProvider, std::enable_shared_from_this<AssetProvider> {
  explicit AssetProvider(IFileProviderPtr fileProvider) : fileProvider_(std::move(fileProvider)) {}

  void registerAssetLoader(IAssetLoaderPtr loader) { assetLoaders_[loader->loadedType()] = std::move(loader); }

  auto load(std::string const& fn, std::type_index typeId) -> IAssetPtr override;

  auto loadRawAsset(std::string const& fn) const -> std::string override;

 private:
  std::shared_ptr<IFileProvider> fileProvider_;
  std::unordered_map<std::type_index, IAssetLoaderPtr> assetLoaders_;
  std::unordered_map<std::string, IWeakAssetPtr> assetsCache_;
};
} // namespace ew
