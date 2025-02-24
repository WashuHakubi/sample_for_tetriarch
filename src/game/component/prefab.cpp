/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "game/component/prefab.h"
#include "engine/asset_database.h"
#include "game/go/scene.h"

EWOK_REGISTRATION {
  using namespace ewok;

  Register::class_<Prefab>("Prefab")
      .addBaseClass<ComponentBase>()
      .field(&Prefab::loadOnAttach_, "loadOnAttach")
      .field(&Prefab::prefabName_, "prefab");
}

namespace ewok {
auto Prefab::attachAsync() -> concurrencpp::result<void> {
  if (loadOnAttach_) {
    co_await loadAsync();
  }
}

auto Prefab::loadAsync() -> concurrencpp::result<GameObjectPtr> {
  co_await concurrencpp::resume_on(globalExecutor());
  if (prefab_) {
    co_return prefab_;
  }

  auto scenePtr = co_await assetDatabase()->loadAssetAsync<Scene>(prefabName_);
  object()->addChild(scenePtr);
  prefab_ = scenePtr;
  co_return prefab_;
}

void Prefab::unload() {
  if (!prefab_) {
    return;
  }

  prefab_->parent()->removeChild(prefab_);
  prefab_ = nullptr;
}
} // namespace ewok
