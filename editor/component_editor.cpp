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

void drawStringEditor(void* instance, std::unique_ptr<Field> const& field) {
  auto str = static_cast<std::string*>(field->getValue(instance));
  auto id = "##" + field->name();
  ImGui::InputText(id.c_str(), str);
}

template <ImGuiDataType T>
void drawScalarEditor(void* instance, std::unique_ptr<Field> const& field) {
  auto str = field->getValue(instance);
  auto id = "##" + field->name();
  ImGui::InputScalar(id.c_str(), T, str);
}

void drawGameObjectHandleEditor(
    void* instance, std::unique_ptr<Field> const& field) {
  auto handle = static_cast<GameObjectHandle*>(field->getValue(instance));
  auto target = handle->lock();
  auto initial = target ? target->name() : "<null>";

  auto id = "##" + field->name();
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

using DrawFn = void (*)(void* instance, std::unique_ptr<Field> const& field);

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
  };

  auto it = drawers.find(field->type());

  return it != drawers.end() ? it->second : nullptr;
}
} // namespace

void ComponentEditor::draw(ComponentPtr const& component) {
  auto p = component.get();
  auto cp = Reflection::getClass(p->getComponentType());
  if (not cp) {
    return;
  }

  if (ImGui::BeginTable(p->describe().c_str(), 2)) {
    ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 100);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

    for (auto&& field : cp->fields()) {
      auto drawer = getFieldDrawer(field);
      if (!drawer) {
        continue;
      }

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%s", field->name().c_str());
      ImGui::TableSetColumnIndex(1);
      drawer(p, field);
    }
    ImGui::EndTable();
  }
}
} // namespace ewok
