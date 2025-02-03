/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

// #include <folly/coro/Task.h>

#include "engine/asset_database.h"
#include "engine/i_component_parser.h"
#include "engine/i_file_provider.h"

namespace ewok {
auto AssetDatabase::loadAssetAsync(
    std::type_index assetType, std::string const& name)
    -> concurrencpp::result<IAssetPtr> {
  co_await concurrencpp::resume_on(executor_);

  if (auto assetIt = weakAssets_.find(name); assetIt != weakAssets_.end()) {
    // Get a shared_ptr to the asset, if this returns null the asset has been
    // unloaded
    auto ptr = assetIt->second.lock();
    if (ptr) {
      co_return ptr;
    }

    // Cleanup dead entry
    weakAssets_.erase(assetIt);
  }

  // Get an asset loader for this type
  auto loaderIt = assetLoaders_.find(assetType);
  if (loaderIt == assetLoaders_.end()) {
    throw std::runtime_error("An asset loader does not exist for that type");
  }
  auto const& assetLoader = loaderIt->second;

  // Load the asset file into memory
  auto data = co_await fileProvider_->readFileAsync(name, executor_);

  // and parse the asset. This may trigger additional dependent loads
  auto asset = co_await assetLoader->loadAssetAsync(*this, std::move(data));

  if (!asset) {
    throw std::runtime_error("Asset failed to load.");
  }

  // Cache the asset to avoid loading it again
  weakAssets_.emplace(name, asset);
  co_return asset;
}

auto AssetDatabase::registerAssetLoader(
    std::type_index assetType, IAssetLoaderPtr ptr) -> void {
  if (!assetLoaders_.emplace(assetType, std::move(ptr)).second) {
    throw std::runtime_error("An asset loader already exists for that type.");
  }
}

auto AssetDatabase::getComponentParser(std::string const& name) const
    -> IComponentParserPtr {
  auto parserIt = componentParsers_.find(name);
  if (parserIt == componentParsers_.end()) {
    return nullptr;
  }

  return parserIt->second;
}

void AssetDatabase::registerComponentParser(IComponentParserPtr ptr) {
  auto const& name = ptr->name();
  if (!componentParsers_.emplace(name, std::move(ptr)).second) {
    throw std::runtime_error("Component parser already eixsts with that name.");
  }
}
} // namespace ewok
