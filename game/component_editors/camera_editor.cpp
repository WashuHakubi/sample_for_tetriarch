/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "game/component_editors/camera_editor.h"
#include "engine/object_database.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <rfl.hpp>

namespace ewok {
CameraEditor* CameraEditor::instance() {
  static CameraEditor cameraEditor;
  return &cameraEditor;
}

void CameraEditor::draw(ComponentPtr const& component) {
  auto camera = static_cast<Camera*>(component.get());

  if (ImGui::BeginTable(camera->describe().c_str(), 2)) {
    ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 100);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", "name");
    ImGui::TableSetColumnIndex(1);
    ImGui::InputText("##name", &camera->name_);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", "target");
    ImGui::TableSetColumnIndex(1);
    auto target =
        camera->target() ? camera->target()->name().c_str() : "<null>";
    if (ImGui::BeginCombo("##target", target)) {
      if (ImGui::Selectable("<null>", camera->target() == nullptr)) {
        camera->target_.reset();
      }

      for (auto&& [guid, handle] : objectDatabase()->getObjects()) {
        auto obj = handle.lock();
        if (!obj) {
          continue;
        }

        if (ImGui::Selectable(obj->name().c_str(), obj == camera->target())) {
          camera->target_ = obj;
        }
      }
      ImGui::EndCombo();
    }
    ImGui::EndTable();
  }
}
} // namespace ewok
