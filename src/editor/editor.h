/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "engine/game_object.h"

namespace ewok {
class PropertyDrawer;
using PropertyDrawerPtr = std::shared_ptr<PropertyDrawer>;

class Editor {
 public:
  Editor();

  bool draw(GameObjectPtr const& root);

 private:
  void drawChildNodes(GameObjectPtr const& node);
  void drawSelectedObjectComponents(GameObjectPtr const& node);

 private:
  GameObjectHandle selected_;
};

class PropertyDrawer {
 public:
  static auto getDrawer(std::type_index type) -> PropertyDrawerPtr;
  static void registerDrawer(std::type_index type, PropertyDrawerPtr ptr);

  virtual ~PropertyDrawer() = default;

  virtual void draw(FieldPtr const& field, void* instance);

 protected:
  virtual void onDraw(FieldPtr const& field, void* instance) = 0;

 private:
  friend class Editor;
  static void initialize();

  static std::unique_ptr<std::unordered_map<std::type_index, PropertyDrawerPtr>>
      s_drawerFactories_;
};

} // namespace ewok
