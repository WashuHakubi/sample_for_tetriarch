/*
 * Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "../i_asset_provider.h"

#include <typeindex>

namespace ew {
struct IAssetLoader {
  virtual ~IAssetLoader() = default;

  [[nodiscard]] virtual auto loadedType() const -> std::type_index = 0;

  [[nodiscard]] virtual auto load(IAssetProviderPtr const& provider, const std::string& fn, std::vector<uint8_t> data)
      -> IAssetPtr = 0;
};
using IAssetLoaderPtr = std::shared_ptr<IAssetLoader>;

template <class T>
struct AssetLoader : IAssetLoader {
  [[nodiscard]] auto loadedType() const -> std::type_index override { return typeid(T); }
};
} // namespace ew
