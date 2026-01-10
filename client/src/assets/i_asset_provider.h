/*
 * Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "i_asset.h"

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <vector>

namespace ew {
struct IAssetProvider {
  virtual ~IAssetProvider() = default;

  template <class T>
    requires(std::is_base_of_v<IAsset, T>)
  auto load(std::string const& fn) -> std::shared_ptr<T> {
    auto const asset = load(fn, std::type_index(typeid(T)));
    return std::static_pointer_cast<T>(asset);
  }

  virtual auto load(std::string const& fn, std::type_index typeId) -> IAssetPtr = 0;

  virtual auto loadRawAsset(std::string const& fn) const -> std::vector<uint8_t> = 0;
};
using IAssetProviderPtr = std::shared_ptr<IAssetProvider>;

} // namespace ew
