/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/i_asset.h"
#include "engine/i_asset_loader.h"

namespace ewok {
class AssetDatabase : public std::enable_shared_from_this<AssetDatabase> {
 public:
  explicit AssetDatabase(
      IFileProviderPtr fileProvider,
      std::shared_ptr<concurrencpp::manual_executor> executor)
      : fileProvider_(std::move(fileProvider)),
        executor_(std::move(executor)) {}

  auto getComponentParser(std::string const& name) const -> IComponentParserPtr;

  auto loadAssetAsync(std::type_index assetType, std::string const& name)
      -> concurrencpp::result<IAssetPtr>;

  template <typename TAsset>
  auto loadAssetAsync(std::string const& name)
      -> concurrencpp::result<std::shared_ptr<TAsset>>
    requires(std::is_final_v<TAsset>)
  {
    auto assetPtr = co_await loadAssetAsync(typeid(TAsset), name);
    co_return std::static_pointer_cast<TAsset>(assetPtr);
  }

  void registerAssetLoader(std::type_index type, IAssetLoaderPtr ptr);

  template <typename TAsset, typename TAssetLoader>
  void registerAssetLoader(std::unique_ptr<TAssetLoader> ptr)
    requires(
        std::is_final_v<TAsset> &&
        std::is_base_of_v<ITypedAssetLoader<TAsset>, TAssetLoader>)
  {
    registerAssetLoader(typeid(TAsset), std::move(ptr));
  }

  void registerComponentParser(IComponentParserPtr ptr);

 private:
  IFileProviderPtr fileProvider_;
  std::shared_ptr<concurrencpp::manual_executor> executor_;
  std::unordered_map<std::type_index, IAssetLoaderPtr> assetLoaders_;
  std::unordered_map<std::string, IComponentParserPtr> componentParsers_;
  std::unordered_map<std::string, std::weak_ptr<IAsset>> weakAssets_;
};
} // namespace ewok
