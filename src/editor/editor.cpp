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
void drawComplexObject(ClassPtr const& class_, void* instance);

Editor::Editor() {
  PropertyDrawer::initialize();
}

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

  auto transformClass = Reflection::getClass(typeid(Transform));
  drawComplexObject(transformClass, &node->transform_);

  for (auto&& comp : node->components()) {
    auto compClass = Reflection::getClass(comp->getComponentType());
    if (compClass) {
      drawComplexObject(compClass, comp.get());
    }
  }
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

template <class T>
struct InternalTypedPropertyDrawer : public PropertyDrawer {
  using value_type = T;
};

class StringPropertyDrawer : public InternalTypedPropertyDrawer<std::string> {
 public:
  void onDraw(FieldPtr const& field, void* instance) override {
    auto str = field->getValue<std::string>(instance);
    auto id = std::format("##{}", field->name());
    if (ImGui::InputText(id.c_str(), &str)) {
      field->setValue(instance, str);
    }
  }
};

class BoolPropertyDrawer : public InternalTypedPropertyDrawer<bool> {
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
class ScalarPropertyDrawer : public InternalTypedPropertyDrawer<T> {
 public:
  void onDraw(FieldPtr const& field, void* instance) override {
    auto v = field->getValue<T>(instance);
    auto id = std::format("##{}", field->name());
    if (ImGui::InputScalarN(id.c_str(), V, &v, N)) {
      field->setValue(instance, v);
    }
  }
};

class QuaternionPropertyDrawer : public InternalTypedPropertyDrawer<Quat> {
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

class GameObjectHandlePropertyDrawer
    : public InternalTypedPropertyDrawer<GameObjectHandle> {
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
static auto makeDrawer() {
  return std::pair{
      std::type_index{typeid(typename T::value_type)}, std::make_shared<T>()};
}

std::unique_ptr<std::unordered_map<std::type_index, PropertyDrawerPtr>>
    PropertyDrawer::s_drawerFactories_;

void PropertyDrawer::initialize() {
  if (s_drawerFactories_ != nullptr) {
    return;
  }

  std::unordered_map<std::type_index, PropertyDrawerPtr> drawerFactories = {
      makeDrawer<StringPropertyDrawer>(),
      makeDrawer<BoolPropertyDrawer>(),

      makeDrawer<ScalarPropertyDrawer<int8_t, ImGuiDataType_S8>>(),
      makeDrawer<ScalarPropertyDrawer<int16_t, ImGuiDataType_S16>>(),
      makeDrawer<ScalarPropertyDrawer<int32_t, ImGuiDataType_S32>>(),
      makeDrawer<ScalarPropertyDrawer<int64_t, ImGuiDataType_S64>>(),
      makeDrawer<ScalarPropertyDrawer<uint8_t, ImGuiDataType_U8>>(),
      makeDrawer<ScalarPropertyDrawer<uint16_t, ImGuiDataType_U16>>(),
      makeDrawer<ScalarPropertyDrawer<uint32_t, ImGuiDataType_U32>>(),
      makeDrawer<ScalarPropertyDrawer<uint64_t, ImGuiDataType_U64>>(),
      makeDrawer<ScalarPropertyDrawer<float, ImGuiDataType_Float>>(),
      makeDrawer<ScalarPropertyDrawer<double, ImGuiDataType_Double>>(),

      makeDrawer<ScalarPropertyDrawer<Vec2, ImGuiDataType_Float, 2>>(),
      makeDrawer<ScalarPropertyDrawer<Vec3, ImGuiDataType_Float, 3>>(),
      makeDrawer<ScalarPropertyDrawer<Vec4, ImGuiDataType_Float, 4>>(),

      makeDrawer<ScalarPropertyDrawer<Int8Vec2, ImGuiDataType_S8, 2>>(),
      makeDrawer<ScalarPropertyDrawer<Int8Vec3, ImGuiDataType_S8, 3>>(),
      makeDrawer<ScalarPropertyDrawer<Int8Vec4, ImGuiDataType_S8, 4>>(),

      makeDrawer<ScalarPropertyDrawer<UInt8Vec2, ImGuiDataType_U8, 2>>(),
      makeDrawer<ScalarPropertyDrawer<UInt8Vec3, ImGuiDataType_U8, 3>>(),
      makeDrawer<ScalarPropertyDrawer<UInt8Vec4, ImGuiDataType_U8, 4>>(),

      makeDrawer<QuaternionPropertyDrawer>(),

      makeDrawer<GameObjectHandlePropertyDrawer>(),
  };

  s_drawerFactories_ =
      std::make_unique<std::unordered_map<std::type_index, PropertyDrawerPtr>>(
          std::move(drawerFactories));
}

void PropertyDrawer::registerDrawer(
    std::type_index type, PropertyDrawerPtr ptr) {
  if (!s_drawerFactories_) {
    initialize();
  }

  assert(s_drawerFactories_ != nullptr);
  s_drawerFactories_->emplace(type, std::move(ptr));
}

auto PropertyDrawer::getDrawer(std::type_index type) -> PropertyDrawerPtr {
  assert(s_drawerFactories_ != nullptr);

  auto it = s_drawerFactories_->find(type);
  if (it == s_drawerFactories_->end()) {
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

void drawComplexObject(ClassPtr const& class_, void* instance) {
  auto id = std::format("##{}", reinterpret_cast<size_t>(instance));
  if (ImGui::CollapsingHeader(
          class_->name().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
    auto tableFlags =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
    ImGui::BeginTable(id.c_str(), 2, tableFlags);
    ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 100.0f);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

    for (auto&& field : class_->fields()) {
      auto fieldClass = Reflection::getClass(field->type());
      if (fieldClass) {
        drawComplexObject(fieldClass, field->getValue(instance, field->type()));
        continue;
      }

      auto arrayAdapter = field->getArrayAdapter();
      if (arrayAdapter) {
        auto arrayClass = Reflection::getClass(arrayAdapter->type());
        auto arrayDrawer = PropertyDrawer::getDrawer(arrayAdapter->type());

        // don't know how to draw this array, skip
        if (arrayClass == nullptr && arrayDrawer == nullptr) {
          continue;
        }

        // Draw the field name
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", field->name().c_str());
        ImGui::TableSetColumnIndex(1);
        auto avail = ImGui::GetContentRegionAvail();
        ImGui::SetNextItemWidth(avail.x);

        // Start a nested table with the array values.
        ImGui::BeginTable(id.c_str(), 2, tableFlags);
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

        auto arrayInst = field->getValue(instance, field->type());
        auto synthField = arrayAdapter->getSyntheticField();
        auto len = arrayAdapter->size(arrayInst);

        for (size_t i = 0; i < len; ++i) {
          if (fieldClass) {
            drawComplexObject(arrayClass, arrayAdapter->item(arrayInst, i));
          } else {
            synthField->setIndex(i);
            arrayDrawer->draw(synthField, arrayInst);
          }
        }

        // End nested table.
        ImGui::EndTable();
        ImGui::BeginTable("##array_ops", 2, ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Button("Add Item")) {
          arrayAdapter->resize(arrayInst, len + 1);
        }
        ImGui::TableNextColumn();
        if (ImGui::Button("Remove Last Item")) {
          arrayAdapter->resize(arrayInst, len - 1);
        }
        ImGui::EndTable();
        continue;
      }

      auto drawer = PropertyDrawer::getDrawer(field->type());
      if (drawer) {
        drawer->draw(field, instance);
      }
    }
    ImGui::EndTable();
  }
}
} // namespace ewok