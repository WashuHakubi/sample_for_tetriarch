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

namespace ew {
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
} // namespace ew