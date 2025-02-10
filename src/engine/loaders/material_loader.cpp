/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include "engine/loaders/material_loader.h"
#include "engine/asset_database.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

namespace ewok {
auto MaterialLoader::loadAssetAsync(AssetDatabase& db, std::vector<char> data)
    -> concurrencpp::result<IAssetPtr> {
  auto tree = ryml::parse_in_place(data.data());
  auto root = tree.rootref();

  if (!root.has_child("shader")) {
    std::cerr << "Failed to find shader node in material" << std::endl;
    co_return nullptr;
  }

  std::string shaderName;
  root["shader"] >> shaderName;

  auto shaderData = co_await db.loadRawAsset(shaderName);
  // pretend we compiled shader data.

  auto material = std::make_shared<Material>();
  co_return material;
}
} // namespace ewok
