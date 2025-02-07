/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "engine/object_database.h"
#include "engine/game_object.h"

namespace ewok {
void ObjectDatabase::add(GameObjectPtr const& obj) {
  assert(obj);
  idToHandle_.emplace(obj->id(), obj);
}

void ObjectDatabase::erase(GameObjectPtr const& obj) {
  if (!obj) {
    return;
  }

  idToHandle_.erase(obj->id());
}
auto ObjectDatabase::find(Guid const& id) const -> GameObjectPtr {
  if (auto it = idToHandle_.find(id); it != idToHandle_.end()) {
    auto p = it->second.lock();
    return p;
  }

  return nullptr;
}
} // namespace ewok
