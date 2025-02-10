/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "engine/game_object.h"
#include "engine/i_asset.h"

namespace ewok {
// Represents a serialized tree of game objects
class Scene final : public GameObject, public IAsset {
 public:
  static auto create(Guid id, bool lazyAttach = false)
      -> std::shared_ptr<Scene>;
  using GameObject::GameObject;

 private:
  friend class SceneLoader;

  void onLoadCompleted();
};
} // namespace ewok
