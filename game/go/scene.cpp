/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "game/go/scene.h"
#include "engine/object_database.h"

namespace ewok {
auto Scene::create(Guid id, bool lazyAttach) -> std::shared_ptr<Scene> {
  auto p = std::make_shared<Scene>(ProtectedOnly{}, id, lazyAttach);
  objectDatabase()->add(p);
  return p;
}

void Scene::onLoadCompleted() {}
} // namespace ewok
