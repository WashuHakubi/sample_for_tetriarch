/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "engine/component.h"
#include "engine/game_object.h"

namespace ewok {
class Prefab : public AsyncComponent<Prefab> {
 public:
  auto attachAsync() -> concurrencpp::result<void> override;

  // Attempts to load the prefab if it hasn't already been loaded.
  auto loadAsync() -> concurrencpp::result<GameObjectPtr>;

  // If true then the prefab will be loaded on attach.
  bool loadOnAttach() const { return loadOnAttach_; }

  // Holds a handle to the prefab after it was loaded. The prefab will also be
  // added to the GO that owns this component. Returns nullptr if the prefab has
  // not been loaded.
  auto prefab() const -> GameObjectPtr const& { return prefab_; }

  // Gets the name of the prefab to load.
  auto prefabName() const -> std::string const& { return prefabName_; }

  // Attempts to unload the prefab and remove it from the parent object.
  void unload();

 private:
  friend class PrefabParser;
  void setPrefabName(std::string value) { prefabName_ = std::move(value); }
  void setLoadOnAttach(bool value) { loadOnAttach_ = value; }

  std::string prefabName_;
  bool loadOnAttach_{false};
  GameObjectPtr prefab_;
};
} // namespace ewok
