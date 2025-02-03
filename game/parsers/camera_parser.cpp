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
    std::unordered_map<std::string, GameObjectPtr> const& pathToObject) const {
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

    if (auto it = pathToObject.find(targetObject); it != pathToObject.end()) {
      if (it->second.get() == comp->object()) {
        std::cerr << "Found circular reference for camera target. Skipping"
                  << std::endl;
      } else {
        camera.setTarget(it->second);
      }
    }
  }
}
} // namespace ewok
