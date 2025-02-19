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

  auto transformClass = Reflection::class_(typeid(Transform));
  drawComplexObject(transformClass, &node->transform_);

  for (auto&& comp : node->components()) {
    auto compClass = Reflection::class_(comp->getComponentType());
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
  void onDraw(FieldWrapper const& field, void* instance) override {
    auto v = reinterpret_cast<std::string*>(field.valuePtr(instance));
    auto id = std::format("##{}", field.name());
    ImGui::InputText(id.c_str(), v);
  }
};

class BoolPropertyDrawer : public InternalTypedPropertyDrawer<bool> {
 public:
  void onDraw(FieldWrapper const& field, void* instance) override {
    auto v = reinterpret_cast<bool*>(field.valuePtr(instance));
    auto id = std::format("##{}", field.name());
    ImGui::Checkbox(id.c_str(), v);
  }
};

template <class T, int V, int N = 1>
class ScalarPropertyDrawer : public InternalTypedPropertyDrawer<T> {
 public:
  void onDraw(FieldWrapper const& field, void* instance) override {
    auto v = reinterpret_cast<T*>(field.valuePtr(instance));
    auto id = std::format("##{}", field.name());
    ImGui::InputScalarN(id.c_str(), V, v, N);
  }
};

class QuaternionPropertyDrawer : public InternalTypedPropertyDrawer<Quat> {
 public:
  void onDraw(FieldWrapper const& field, void* instance) override {
    auto q = reinterpret_cast<Quat*>(field.valuePtr(instance));
    Vec3 v = toEuler(*q);
    auto id = std::format("##{}", field.name());
    if (ImGui::InputScalarN(id.c_str(), ImGuiDataType_Float, &v, 3)) {
      *q = fromEuler(v);
    }
  }
};

class GameObjectHandlePropertyDrawer
    : public InternalTypedPropertyDrawer<GameObjectHandle> {
 protected:
  void onDraw(FieldWrapper const& field, void* instance) override {
    auto v = reinterpret_cast<GameObjectHandle*>(field.valuePtr(instance));
    auto target = v->lock();
    auto initial = target ? target->name() : "<null>";

    auto id = std::format("##{}", field.name());
    if (ImGui::BeginCombo(id.c_str(), initial.c_str())) {
      if (ImGui::Selectable("<null>", target == nullptr)) {
        *v = GameObjectHandle{};
      }

      for (auto&& [guid, h] : objectDatabase()->getObjects()) {
        auto obj = h.lock();
        if (!obj) {
          continue;
        }

        if (ImGui::Selectable(obj->name().c_str(), obj == target)) {
          *v = GameObjectHandle{obj};
        }
      }
      ImGui::EndCombo();
    }
  }
};

class EnumPropertyDrawer : public PropertyDrawer {
  void onDraw(FieldWrapper const& field, void* instance) override {
    auto enumType = Reflection::enum_(field.type());
    assert(enumType);

    auto v = enumType->name(field.valuePtr(instance), field.type());
    std::string initial = v.value_or("");

    auto id = std::format("##{}", field.name());
    if (ImGui::BeginCombo(id.c_str(), initial.c_str())) {
      auto vals = enumType->values();
      for (auto&& [val, name] : vals) {
        if (ImGui::Selectable(name.c_str(), name == initial)) {
          enumType->setValue(field.valuePtr(instance), val);
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
    auto enumType = Reflection::enum_(type);
    if (enumType) {
      // Check if it's an enumeration. If it is register a drawer for it
      it = s_drawerFactories_
               ->emplace(type, std::make_shared<EnumPropertyDrawer>())
               .first;
    } else {
      // Otherwise we don't know what this is, so ignore it.
      return nullptr;
    }
  }

  return it->second;
}

void PropertyDrawer::draw(FieldWrapper const& field, void* instance) {
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("%s", field.name().c_str());
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
      auto fieldClass = Reflection::class_(field->type());
      if (fieldClass) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", field->name().c_str());
        ImGui::TableSetColumnIndex(1);
        auto avail = ImGui::GetContentRegionAvail();
        ImGui::SetNextItemWidth(avail.x);

        drawComplexObject(fieldClass, field->valuePtr(instance));
        continue;
      }

      if (auto arrayType = field->asArray(); arrayType != nullptr) {
        auto elemClass = arrayType->getElemClass();
        auto elemDrawer = PropertyDrawer::getDrawer(arrayType->getElemType());

        // don't know how to draw this array, skip
        if (elemClass == nullptr && elemDrawer == nullptr) {
          continue;
        }

        //   // Draw the field name
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

        // auto arrayInst = field->getValue(instance, field->type());
        // auto synthField = arrayAdapter->getSyntheticField();
        auto len = arrayType->getSize(instance);

        for (size_t i = 0; i < len; ++i) {
          if (fieldClass) {
            drawComplexObject(
                elemClass, arrayType->valueAtIndexPtr(instance, i));
          } else {
            // synthField->setIndex(i);
            elemDrawer->draw(ArrayFieldWrapper(field, arrayType, i), instance);
          }
        }

        // End nested table.
        ImGui::EndTable();
        ImGui::BeginTable("##array_ops", 2, ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Button("Add Item")) {
          arrayType->setSize(instance, len + 1);
        }
        ImGui::TableNextColumn();
        if (ImGui::Button("Remove Last Item")) {
          arrayType->setSize(instance, len - 1);
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