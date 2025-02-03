/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "game/parsers/prefab_parser.h"
#include "game/component/prefab.h"

namespace ewok {
auto PrefabParser::create() const -> ComponentPtr {
  return std::make_shared<Prefab>();
}

auto PrefabParser::name() const -> std::string {
  return Prefab::typeName();
}

void PrefabParser::parse(
    ComponentPtr const& comp,
    ryml::ConstNodeRef componentNode,
    std::unordered_map<std::string, GameObjectPtr> const& pathToObject) const {
  assert((componentNode["type"].val() == name()));
  auto& prefab = static_cast<Prefab&>(*comp);

  // Find the target the camera should point at, if it doesn't exist target
  // will be nullptr.
  if (componentNode.has_child("prefab")) {
    std::string targetScene;
    componentNode["prefab"] >> targetScene;

    prefab.setPrefabName(std::move(targetScene));
  }

  if (componentNode.has_child("loadOnAttach")) {
    bool loadOnAttach;
    componentNode["loadOnAttach"] >> loadOnAttach;
    prefab.setLoadOnAttach(loadOnAttach);
  }
}
} // namespace ewok
