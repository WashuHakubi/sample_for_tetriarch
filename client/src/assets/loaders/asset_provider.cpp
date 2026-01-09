/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "asset_provider.h"

namespace ew {
auto AssetProvider::load(std::string const& fn, std::type_index typeId) -> IAssetPtr {
  if (auto const it = assetsCache_.find(fn); it != assetsCache_.end()) {
    if (auto asset = it->second.lock()) {
      return asset;
    }

    // Cleanup dead entry in the asset cache
    assetsCache_.erase(it);
  }

  auto const data = fileProvider_->load(fn);
  auto const asset = assetLoaders_.at(typeId)->load(shared_from_this(), fn, data);
  assetsCache_.emplace(fn, asset);
  return asset;
}

auto AssetProvider::loadRawAsset(std::string const& fn) const -> std::string {
  return fileProvider_->load(fn);
}
} // namespace ew
