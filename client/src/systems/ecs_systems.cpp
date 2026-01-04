/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "ecs_systems.h"

namespace ew {
void EcsSystems::clear() {
  updateSystems_.clear();
  renderSystems_.clear();
  messageHandlers_.clear();
  typeToSystem_.clear();

  // Remove the systems in the opposite order they were added, this should ensure that any ordering related destruction
  // semantics are respected.
  while (!systems_.empty()) {
    systems_.pop_back();
  }
}

void EcsSystems::render(float dt) const {
  for (auto&& system : renderSystems_) {
    system(dt);
  }
}

void EcsSystems::update(float dt) const {
  for (auto&& system : updateSystems_) {
    system(dt);
  }
}

void EcsSystems::handleMessage(GameThreadMsg const& msg) const {
  for (auto&& system : messageHandlers_) {
    system(msg);
  }
}
} // namespace ew
