/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "editor/editor.h"
#include "engine/component.h"
#include "engine/object_database.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

namespace ewok {
void Editor::drawChildNodes(GameObjectPtr const& node) {
  for (auto&& child : node->children()) {
    auto name = std::format(
        "{} ({})", child->name(), reinterpret_cast<size_t>(child.get()));
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;

    auto selected = selected_.lock();
    if (selected == child) {
      flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (child->children().empty()) {
      flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool active = child->active();
    if (!active) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1));
    }

    if (ImGui::TreeNodeEx(name.c_str(), flags)) {
      if (!active) {
        ImGui::PopStyleColor();
      }
      if (ImGui::IsItemClicked()) {
        if (selected != child && selected != nullptr) {
          selectedView_ = nullptr;
        }
        selected_ = child;
      }

      drawChildNodes(child);
      ImGui::TreePop();
    } else {
      if (!active) {
        ImGui::PopStyleColor();
      }
    }
  }
}

void Editor::drawSelectedObjectComponents(GameObjectPtr const& node) {
  if (!node) {
    return;
  }

  if (!selectedView_) {
    selectedView_ = std::make_shared<EditorComponentsView>(node);
  }

  selectedView_->draw();
}

bool Editor::draw(GameObjectPtr const& root) {
  bool exit = false;
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit", "x")) {
        exit = true;
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
  ImGui::Begin("Object Tree", nullptr, ImGuiWindowFlags_NoBackground);
  if (ImGui::TreeNodeEx("Root", ImGuiTreeNodeFlags_DefaultOpen)) {
    drawChildNodes(root);
    ImGui::TreePop();
  }
  ImGui::End();

  ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
  ImGui::Begin("Component View", nullptr, ImGuiWindowFlags_NoBackground);

  drawSelectedObjectComponents(selected_.lock());

  ImGui::End();

  return exit;
}

EditorComponentsView::EditorComponentsView(GameObjectPtr const& node) {
  auto class_ = Reflection::getClass<GameObject>();
  views_.emplace_back(node.get(), class_);

  for (auto&& comp : node->components()) {
    auto compClass = Reflection::getClass(comp->getComponentType());
    if (compClass) {
      views_.emplace_back(comp.get(), compClass);
    }
  }
}

void EditorComponentsView::draw() {
  for (auto&& view : views_) {
    view.draw();
    ImGui::Separator();
  }
}

ComponentView::ComponentView(void* p, ClassPtr const& class_)
    : instance_(p), class_(class_) {
  id_ = std::format("##{}", reinterpret_cast<size_t>(instance_));
  buildCompositeFields(p, class_);
}

void ComponentView::buildCompositeFields(void* p, ClassPtr const& class_) {
  for (auto&& field : class_->fields()) {
    auto drawer = PropertyDrawer::getDrawer(field->type());
    if (!drawer) {
      auto childClass = field->getClass();
      if (childClass) {
        auto val = const_cast<void*>(field->getValue(p, childClass->type()));
        buildCompositeFields(val, childClass);
      }
      continue;
    }

    fieldDrawers_.emplace_back(p, field, std::move(drawer));
  }
}

void ComponentView::draw() {
  if (ImGui::CollapsingHeader(
          class_->name().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
    auto tableFlags =
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable;
    if (ImGui::BeginTable(id_.c_str(), 2, tableFlags)) {
      ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 100);
      ImGui::TableSetupColumn(
          "value", ImGuiTableColumnFlags_WidthStretch, 1.0f);

      for (auto&& [p, field, drawer] : fieldDrawers_) {
        drawer->draw(field, p);
      }

      ImGui::EndTable();
    }
  }
}

class StringPropertyDrawer : public PropertyDrawer {
 public:
  void onDraw(FieldPtr const& field, void* instance) override {
    auto str = field->getValue<std::string>(instance);
    auto id = std::format("##{}", field->name());
    if (ImGui::InputText(id.c_str(), &str)) {
      field->setValue(instance, str);
    }
  }
};

class BoolPropertyDrawer : public PropertyDrawer {
 public:
  void onDraw(FieldPtr const& field, void* instance) override {
    auto v = field->getValue<bool>(instance);
    auto id = std::format("##{}", field->name());
    if (ImGui::Checkbox(id.c_str(), &v)) {
      field->setValue(instance, v);
    }
  }
};

template <class T, int V, int N = 1>
class ScalarPropertyDrawer : public PropertyDrawer {
 public:
  void onDraw(FieldPtr const& field, void* instance) override {
    auto v = field->getValue<T>(instance);
    auto id = std::format("##{}", field->name());
    if (ImGui::InputScalarN(id.c_str(), V, &v, N)) {
      field->setValue(instance, v);
    }
  }
};

class QuaternionPropertyDrawer : public PropertyDrawer {
 public:
  void onDraw(FieldPtr const& field, void* instance) override {
    auto q = field->getValue<Quat>(instance);
    Vec3 v = toEuler(q);
    auto id = std::format("##{}", field->name());
    if (ImGui::InputScalarN(id.c_str(), ImGuiDataType_Float, &v, 3)) {
      field->setValue(instance, fromEuler(v));
    }
  }
};

class GameObjectHandlePropertyDrawer : public PropertyDrawer {
 protected:
  void onDraw(FieldPtr const& field, void* instance) override {
    auto handle = field->getValue<GameObjectHandle>(instance);
    auto target = handle.lock();
    auto initial = target ? target->name() : "<null>";

    auto id = std::format("##{}", field->name());
    if (ImGui::BeginCombo(id.c_str(), initial.c_str())) {
      if (ImGui::Selectable("<null>", target == nullptr)) {
        field->setValue(instance, GameObjectHandle{});
      }

      for (auto&& [guid, h] : objectDatabase()->getObjects()) {
        auto obj = h.lock();
        if (!obj) {
          continue;
        }

        if (ImGui::Selectable(obj->name().c_str(), obj == target)) {
          field->setValue(instance, GameObjectHandle{obj});
        }
      }
      ImGui::EndCombo();
    }
  }
};

template <class T>
static PropertyDrawerPtr makeDrawer() {
  return std::make_shared<T>();
}
auto PropertyDrawer::getDrawer(std::type_index type) -> PropertyDrawerPtr {
  static const std::unordered_map<std::type_index, PropertyDrawerPtr>
      drawerFactories = {
          {typeid(std::string), makeDrawer<StringPropertyDrawer>()},
          {typeid(bool), makeDrawer<BoolPropertyDrawer>()},

          {typeid(int8_t),
           makeDrawer<ScalarPropertyDrawer<int8_t, ImGuiDataType_S8>>()},
          {typeid(int16_t),
           makeDrawer<ScalarPropertyDrawer<int16_t, ImGuiDataType_S16>>()},
          {typeid(int32_t),
           makeDrawer<ScalarPropertyDrawer<int32_t, ImGuiDataType_S32>>()},
          {typeid(int64_t),
           makeDrawer<ScalarPropertyDrawer<int64_t, ImGuiDataType_S64>>()},
          {typeid(uint8_t),
           makeDrawer<ScalarPropertyDrawer<uint8_t, ImGuiDataType_U8>>()},
          {typeid(uint16_t),
           makeDrawer<ScalarPropertyDrawer<uint16_t, ImGuiDataType_U16>>()},
          {typeid(uint32_t),
           makeDrawer<ScalarPropertyDrawer<uint32_t, ImGuiDataType_U32>>()},
          {typeid(uint64_t),
           makeDrawer<ScalarPropertyDrawer<uint64_t, ImGuiDataType_U64>>()},
          {typeid(float),
           makeDrawer<ScalarPropertyDrawer<float, ImGuiDataType_Float>>()},
          {typeid(double),
           makeDrawer<ScalarPropertyDrawer<double, ImGuiDataType_Double>>()},

          {typeid(Vec2),
           makeDrawer<ScalarPropertyDrawer<Vec2, ImGuiDataType_Float, 2>>()},
          {typeid(Vec3),
           makeDrawer<ScalarPropertyDrawer<Vec3, ImGuiDataType_Float, 3>>()},
          {typeid(Vec4),
           makeDrawer<ScalarPropertyDrawer<Vec4, ImGuiDataType_Float, 4>>()},

          {typeid(Int8Vec2),
           makeDrawer<ScalarPropertyDrawer<Int8Vec2, ImGuiDataType_S8, 2>>()},
          {typeid(Int8Vec3),
           makeDrawer<ScalarPropertyDrawer<Int8Vec3, ImGuiDataType_S8, 3>>()},
          {typeid(Int8Vec4),
           makeDrawer<ScalarPropertyDrawer<Int8Vec4, ImGuiDataType_S8, 4>>()},

          {typeid(UInt8Vec2),
           makeDrawer<ScalarPropertyDrawer<UInt8Vec2, ImGuiDataType_U8, 2>>()},
          {typeid(UInt8Vec3),
           makeDrawer<ScalarPropertyDrawer<UInt8Vec3, ImGuiDataType_U8, 3>>()},
          {typeid(UInt8Vec4),
           makeDrawer<ScalarPropertyDrawer<UInt8Vec4, ImGuiDataType_U8, 4>>()},

          {typeid(GameObjectHandle),
           makeDrawer<GameObjectHandlePropertyDrawer>()},
      };

  auto it = drawerFactories.find(type);
  if (it == drawerFactories.end()) {
    return nullptr;
  }

  return it->second;
}

void PropertyDrawer::draw(FieldPtr const& field, void* instance) {
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("%s", field->name().c_str());
  ImGui::TableSetColumnIndex(1);
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::SetNextItemWidth(avail.x);
  onDraw(field, instance);
}
} // namespace ewok