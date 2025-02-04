/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/i_asset_loader.h"
#include "engine/renderables/mesh.h"

namespace ewok {
class MeshLoader : public ITypedAssetLoader<Mesh> {
  auto loadAssetAsync(AssetDatabase& db, std::vector<char> data)
      -> concurrencpp::result<IAssetPtr>;
};
} // namespace ewok
