/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "asset_provider.h"

namespace ew {
AssetProvider::AssetProvider(IFileProviderPtr fileProvider, std::shared_ptr<coro::io_scheduler> assetScheduler)
    : assetScheduler_(std::move(assetScheduler))
    , fileProvider_(std::move(fileProvider)) {}

auto AssetProvider::load(std::string const& fn, std::type_index typeId) -> IAssetPtr {
  if (auto const it = assetsCache_.find(fn); it != assetsCache_.end()) {
    if (auto asset = it->second.lock()) {
      return asset;
    }

    // Cleanup dead entry in the asset cache
    assetsCache_.erase(it);
  }

  auto const data = loadRawAsset(fn);
  auto const asset = assetLoaders_.at(typeId)->load(shared_from_this(), fn, std::move(data));
  assetsCache_.emplace(fn, asset);
  return asset;
}

auto AssetProvider::loadRawAsset(std::string const& fn) const -> std::vector<uint8_t> {
  return fileProvider_->load(fn);
}

auto AssetProvider::loadAsync(std::string const& fn, std::type_index typeId) -> coro::task<IAssetPtr> {
  co_await assetScheduler_->schedule();
  if (auto const it = assetsCache_.find(fn); it != assetsCache_.end()) {
    if (auto asset = it->second.lock()) {
      co_return asset;
    }

    // Cleanup dead entry in the asset cache
    assetsCache_.erase(it);
  }

  auto const data = co_await loadRawAssetAsync(fn);
  if (!data.has_value()) {
    co_return nullptr;
  }

  auto const asset = assetLoaders_.at(typeId)->load(shared_from_this(), fn, data.value());
  assetsCache_.emplace(fn, asset);
  co_return asset;
}

auto AssetProvider::loadRawAssetAsync(std::string const& fn) const
    -> coro::task<std::expected<std::vector<uint8_t>, FileError>> {
  auto data = co_await fileProvider_->loadAsync(fn);
  // Switch back to the asset scheduler as the file provider may have switched to another scheduler
  co_await assetScheduler_->schedule();
  co_return data;
}
} // namespace ew
