/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include "engine/loaders/mesh_loader.h"
#include "engine/asset_database.h"
#include "engine/renderables/material.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

namespace ewok {
auto MeshLoader::loadAssetAsync(AssetDatabase& db, std::vector<char> data)
    -> concurrencpp::result<IAssetPtr> {
  auto tree = ryml::parse_in_place(data.data());
  auto root = tree.rootref();

  if (!root.has_child("material")) {
    std::cerr << "Failed to find material node in mesh" << std::endl;
    co_return nullptr;
  }

  std::string matName;
  root["shader"] >> matName;

  auto shaderData = co_await db.loadAssetAsync<Material>(matName);

  std::string verts;
  root["vertices"] >> verts;

  std::string indices;
  root["indices"] >> indices;

  auto mesh = std::make_shared<Mesh>(
      BufferHandle{}, BufferHandle{}, std::move(shaderData));
  co_return mesh;
}
} // namespace ewok
