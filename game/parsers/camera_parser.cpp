/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "game/parsers/camera_parser.h"
#include "game/component/camera.h"

namespace ewok {
auto CameraParser::create() const -> ComponentPtr {
  return std::make_shared<Camera>();
}

auto CameraParser::name() const -> std::string {
  return Camera::typeName();
}

void CameraParser::parse(
    ComponentPtr const& comp,
    ryml::ConstNodeRef componentNode,
    GameObjectPtr const& root) const {
  assert((componentNode["type"].val() == name()));
  std::string name;
  componentNode["name"] >> name;

  auto& camera = static_cast<Camera&>(*comp);
  camera.setName(name);

  // Find the target the camera should point at, if it doesn't exist target
  // will be nullptr.
  if (componentNode.has_child("target")) {
    std::string targetObject;
    componentNode["target"] >> targetObject;

    std::stringstream test(targetObject);
    std::string segment;
    std::vector<std::string> path;

    while (std::getline(test, segment, '/')) {
      path.push_back(segment);
    }

    camera.setTarget(root->findDescendant(path));
  }
}
} // namespace ewok
