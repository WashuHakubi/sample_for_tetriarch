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
// Fake camera, not a real one, just showing off some component stuff.
class Camera : public Component<Camera> {
 public:
  void attach() override;

  constexpr auto name() -> std::string const& { return name_; }
  void setName(std::string name) { name_ = std::move(name); }

  auto target() const -> GameObjectPtr { return target_.lock(); }
  void setTarget(GameObjectPtr target) { target_ = std::move(target); }

 private:
  std::string name_;

  // Weak handle to a game object, this prevents circular references that can
  // cause memory leaks.
  GameObjectHandle target_;
};
} // namespace ewok
