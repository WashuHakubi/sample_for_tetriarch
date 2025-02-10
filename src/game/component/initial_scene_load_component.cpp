/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "game/component/initial_scene_load_component.h"

#include "game/go/scene.h"

#include "engine/asset_database.h"
#include "engine/game_object.h"

#include <iostream>

namespace ewok {
concurrencpp::result<void> InitialSceneLoadComponent::attachAsync() {
  try {
    auto const& assetDb = assetDatabase();
    auto go = co_await assetDb->loadAssetAsync<Scene>("start.yaml");
    object()->addChild(go);
  } catch (std::exception const& ex) {
    std::cerr << "Failed to load initial scene: " << ex.what() << std::endl;
    exit(-1);
  }

  // Destroy this component on the first update after the load has completed.
  object()->queueRemoveComponent(shared_from_this());
}
} // namespace ewok
