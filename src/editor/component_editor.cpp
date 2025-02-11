/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "editor/component_editor.h"
#include "engine/component.h"
#include "engine/game_object.h"
#include "engine/object_database.h"
#include "engine/reflection/reflection.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

namespace ewok {
namespace {
void drawName(std::unique_ptr<Field> const& field) {
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::Text("%s", field->name().c_str());
  ImGui::TableSetColumnIndex(1);
}

void drawStringEditor(void* instance, std::unique_ptr<Field> const& field) {
  auto str = static_cast<std::string*>(field->getValue(instance));
  auto id = std::format("##{}", reinterpret_cast<size_t>(str));
  drawName(field);
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::SetNextItemWidth(avail.x);
  ImGui::InputText(id.c_str(), str);
}

template <int T>
void drawScalarEditor(void* instance, std::unique_ptr<Field> const& field) {
  auto str = field->getValue(instance);
  auto id = std::format("##{}", reinterpret_cast<size_t>(str));
  drawName(field);
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::SetNextItemWidth(avail.x);
  ImGui::InputScalar(id.c_str(), T, str);
}

template <int T, size_t N>
void drawScalarEditorN(void* instance, std::unique_ptr<Field> const& field) {
  auto str = field->getValue(instance);
  auto id = std::format("##{}", reinterpret_cast<size_t>(str));
  drawName(field);
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::SetNextItemWidth(avail.x);
  ImGui::InputScalarN(id.c_str(), T, str, N);
}

void drawQuaternionEditor(void* instance, std::unique_ptr<Field> const& field) {
  auto q = reinterpret_cast<Quat*>(field->getValue(instance));
  auto id = std::format("##{}", reinterpret_cast<size_t>(q));
  drawName(field);
  Vec3 v = toEuler(*q);
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::SetNextItemWidth(avail.x);
  if (ImGui::InputScalarN(id.c_str(), ImGuiDataType_Float, &v, 3)) {
    *q = fromEuler(v);
  }
}

void drawGameObjectHandleEditor(
    void* instance, std::unique_ptr<Field> const& field) {
  auto handle = static_cast<GameObjectHandle*>(field->getValue(instance));
  auto target = handle->lock();
  auto initial = target ? target->name() : "<null>";

  drawName(field);
  auto id = std::format("##{}", reinterpret_cast<size_t>(handle));
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::SetNextItemWidth(avail.x);
  if (ImGui::BeginCombo(id.c_str(), initial.c_str())) {
    if (ImGui::Selectable("<null>", target == nullptr)) {
      handle->reset();
    }

    for (auto&& [guid, h] : objectDatabase()->getObjects()) {
      auto obj = h.lock();
      if (!obj) {
        continue;
      }

      if (ImGui::Selectable(obj->name().c_str(), obj == target)) {
        *handle = obj;
      }
    }
    ImGui::EndCombo();
  }
}
} // namespace

DrawFn getFieldDrawer(std::unique_ptr<Field> const& field) {
  static std::unordered_map<std::type_index, DrawFn> drawers{
      {typeid(std::string), drawStringEditor},
      {typeid(GameObjectHandle), drawGameObjectHandleEditor},
      {typeid(float), drawScalarEditor<ImGuiDataType_Float>},
      {typeid(double), drawScalarEditor<ImGuiDataType_Double>},
      {typeid(int16_t), drawScalarEditor<ImGuiDataType_S16>},
      {typeid(int32_t), drawScalarEditor<ImGuiDataType_S32>},
      {typeid(int64_t), drawScalarEditor<ImGuiDataType_S64>},
      {typeid(uint16_t), drawScalarEditor<ImGuiDataType_U16>},
      {typeid(uint32_t), drawScalarEditor<ImGuiDataType_U32>},
      {typeid(uint64_t), drawScalarEditor<ImGuiDataType_U64>},
      {typeid(Vec2), drawScalarEditorN<ImGuiDataType_Float, 2>},
      {typeid(Vec3), drawScalarEditorN<ImGuiDataType_Float, 3>},
      {typeid(Vec4), drawScalarEditorN<ImGuiDataType_Float, 4>},
      {typeid(Quat), drawQuaternionEditor},
  };

  auto it = drawers.find(field->type());
  return it != drawers.end() ? it->second : nullptr;
}

void drawChildCompositeType(void* p, Class const* class_) {
  for (auto&& field : class_->fields()) {
    auto drawer = getFieldDrawer(field);
    if (!drawer) {
      auto childClass = field->getClass();
      if (childClass) {
        auto val = field->getValue(p);
        drawChildCompositeType(val, childClass);
      }
      continue;
    }

    drawer(p, field);
  }
}

void drawCompositeType(void* p, Class const* class_) {
  ImGui::Text("%s", class_->name().c_str());
  auto id = std::format("##{}", reinterpret_cast<size_t>(p));
  if (ImGui::BeginTable(id.c_str(), 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 100);
    // ImGui::TableSetupColumn("value",
    // ImGuiTableColumnFlags_WidthStretch, 1.0f);

    drawChildCompositeType(p, class_);

    ImGui::EndTable();
  }
}

void ComponentEditor::draw(ComponentPtr const& component) {
  auto p = component.get();
  auto cp = Reflection::getClass(p->getComponentType());
  if (not cp) {
    return;
  }

  drawCompositeType(p, cp);
}
} // namespace ewok
