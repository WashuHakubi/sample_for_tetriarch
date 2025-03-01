/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <concurrencpp/concurrencpp.h>

#include "engine/forward.h"
#include "engine/i_asset.h"

namespace ewok {
struct IAssetLoader {
  virtual ~IAssetLoader() = default;

  virtual auto loadAssetAsync(AssetDatabase& db, std::vector<char> data)
      -> concurrencpp::result<IAssetPtr> = 0;

 private:
  template <IsAssetType T>
  friend struct ITypedAssetLoader;

  // Always derive from an ITypedAssetLoader
  IAssetLoader() = default;
};

template <IsAssetType T>
struct ITypedAssetLoader : IAssetLoader {};
} // namespace ewok
