/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "i_file_provider.h"

namespace ew {
struct AssetProvider;
using AssetProviderPtr = std::shared_ptr<AssetProvider>;

struct IAsset {
  virtual ~IAsset() = default;
};
using IAssetPtr = std::shared_ptr<IAsset>;
using IWeakAssetPtr = std::weak_ptr<IAsset>;

template <class T>
struct Asset : IAsset {
  using Ptr = std::shared_ptr<T>;

  Asset() { static_assert(std::is_base_of_v<Asset, T>); }
};

struct IAssetLoader {
  virtual ~IAssetLoader() = default;

  [[nodiscard]] virtual auto loadedType() const -> std::type_index = 0;

  [[nodiscard]] virtual auto load(AssetProviderPtr const& provider, const std::string& fn, std::string const& data)
      -> IAssetPtr = 0;
};
using IAssetLoaderPtr = std::shared_ptr<IAssetLoader>;

template <class T>
struct AssetLoader : IAssetLoader {
  [[nodiscard]] auto loadedType() const -> std::type_index override { return typeid(T); }
};

struct AssetProvider : std::enable_shared_from_this<AssetProvider> {
  explicit AssetProvider(IFileProviderPtr fileProvider) : fileProvider_(std::move(fileProvider)) {}

  void registerAssetLoader(IAssetLoaderPtr loader) { assetLoaders_[loader->loadedType()] = std::move(loader); }

  template <class T>
    requires(std::is_base_of_v<IAsset, T>)
  auto load(std::string const& fn) -> std::shared_ptr<T> {
    auto const asset = load(fn, std::type_index(typeid(T)));
    return std::static_pointer_cast<T>(asset);
  }

  auto load(std::string const& fn, std::type_index typeId) -> IAssetPtr;

  auto loadRawAsset(std::string const& fn) const -> std::string { return fileProvider_->load(fn); }

 private:
  std::shared_ptr<IFileProvider> fileProvider_;
  std::unordered_map<std::type_index, IAssetLoaderPtr> assetLoaders_;
  std::unordered_map<std::string, IWeakAssetPtr> assetsCache_;
};
} // namespace ew