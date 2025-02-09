/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "engine/game_object.h"

namespace ewok {
class Editor {
 public:
  void draw(GameObjectPtr const& root);

 private:
  void drawChildNodes(GameObjectPtr const& node);
  void drawSelectedObjectComponents(GameObjectPtr const& node);

 private:
  GameObjectPtr selected_;
};

} // namespace ewok