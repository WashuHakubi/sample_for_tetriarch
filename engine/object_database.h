/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"

namespace ewok {
class ObjectDatabase {
 public:
  void add(GameObjectPtr const& obj);

  void erase(GameObjectPtr const& obj);

  auto find(Guid const& id) const -> GameObjectPtr;

 private:
  std::unordered_map<Guid, GameObjectHandle> idToHandle_;
};
} // namespace ewok
