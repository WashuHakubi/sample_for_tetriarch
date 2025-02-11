/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "editor/editor.h"
#include "editor/component_editor.h"
#include "engine/component.h"

#include "imgui.h"

namespace ewok {
void Editor::drawChildNodes(GameObjectPtr const& node) {
  for (auto&& child : node->children()) {
    auto name = std::format(
        "{} ({})", child->name(), reinterpret_cast<size_t>(child.get()));
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;

    if (selected_ == child) {
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

  auto goc = Reflection::getClass<GameObject>();
  drawCompositeType(node.get(), goc);

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
  ImGui::Begin("Object Tree", nullptr, ImGuiWindowFlags_NoBackground);
  if (ImGui::TreeNodeEx("Root", ImGuiTreeNodeFlags_DefaultOpen)) {
    drawChildNodes(root);
    ImGui::TreePop();
  }
  ImGui::End();

  ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
  ImGui::Begin("Component View", nullptr, ImGuiWindowFlags_NoBackground);

  drawSelectedObjectComponents(selected_);

  ImGui::End();

  return exit;
}
} // namespace ewok