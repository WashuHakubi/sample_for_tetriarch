/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "editor/editor.h"
#include "editor/i_component_editor.h"
#include "engine/component.h"

#include "imgui.h"

namespace ewok {
void Editor::drawChildNodes(GameObjectPtr const& node) {
  for (auto&& child : node->children()) {
    auto name = std::format(
        "{} ({})", child->name(), reinterpret_cast<size_t>(child.get()));
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;

    if (selected_ == child) {
      flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (child->children().empty()) {
      flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (ImGui::TreeNodeEx(name.c_str(), flags)) {
      if (ImGui::IsItemClicked()) {
        selected_ = child;
      }

      drawChildNodes(child);
      ImGui::TreePop();
    }
  }
}

void Editor::drawSelectedObjectComponents(GameObjectPtr const& node) {
  if (!node) {
    return;
  }

  for (auto&& comp : node->components()) {
    auto editor = comp->getComponentEditor();
    if (editor) {
      editor->draw(comp);
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
  ImGui::Begin("Object Tree");
  if (ImGui::TreeNodeEx("Root", ImGuiTreeNodeFlags_DefaultOpen)) {
    drawChildNodes(root);
    ImGui::TreePop();
  }
  ImGui::End();

  ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
  ImGui::Begin("Component View");

  drawSelectedObjectComponents(selected_);

  ImGui::End();

  return exit;
}
} // namespace ewok