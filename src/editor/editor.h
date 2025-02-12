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

class ComponentView {
 public:
  ComponentView(void* p, ClassPtr const& class_);

  void draw();

 private:
  void buildCompositeFields(void* p, ClassPtr const& class_);

  void* instance_;
  ClassPtr class_;
  std::string id_;
  std::vector<std::tuple<void*, FieldPtr, PropertyDrawerPtr>> fieldDrawers_;
};

class EditorComponentsView {
 public:
  EditorComponentsView(GameObjectPtr const& node);

  void draw();

 private:
  std::vector<ComponentView> views_;
};

class Editor {
 public:
  bool draw(GameObjectPtr const& root);

 private:
  void drawChildNodes(GameObjectPtr const& node);
  void drawSelectedObjectComponents(GameObjectPtr const& node);

 private:
  GameObjectHandle selected_;
  std::shared_ptr<EditorComponentsView> selectedView_;
};

class PropertyDrawer {
 public:
  static auto getDrawer(std::type_index type) -> PropertyDrawerPtr;

  virtual ~PropertyDrawer() = default;

  virtual void draw(FieldPtr const& field, void* instance);

 protected:
  virtual void onDraw(FieldPtr const& field, void* instance) = 0;
};
} // namespace ewok