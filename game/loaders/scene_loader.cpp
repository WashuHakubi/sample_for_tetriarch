/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "game/loaders/scene_loader.h"
#include "engine/asset_database.h"
#include "engine/i_component_parser.h"
#include "engine/object_database.h"

#include <iostream>

namespace ewok {
namespace {
class Loader {
 public:
  // Recursively reads game objects from a YAML tree.
  auto loadObject(
      AssetDatabase& db,
      ryml::ConstNodeRef objectNode,
      GameObjectPtr const& result) -> concurrencpp::result<void> {
    std::string name;

    // All game objects must have a name.
    assert(objectNode.has_child("name"));
    objectNode["name"] >> name;

    // Names cannot contain a '/'
    assert(name.find_first_of('/') == std::string::npos);
    result->setName(name);

    if (objectNode.has_child("active")) {
      bool active;
      objectNode["active"] >> active;
      result->setActive(active);
    }

    if (objectNode.has_child("transform")) {
      auto transNode = objectNode["transform"];
      // We expect 9 children, 3 for position, 3 for rotation and 3 for scale
      assert(transNode.num_children() == 9);
      Transform transform;
      transNode[0] >> transform.position.x;
      transNode[1] >> transform.position.y;
      transNode[2] >> transform.position.z;

      float roll;
      float pitch;
      float yaw;
      transNode[3] >> roll;
      transNode[4] >> pitch;
      transNode[5] >> yaw;
      auto eulerAngles = asEuler(roll, pitch, yaw);
      transform.rotation = fromEuler(eulerAngles);

      transNode[6] >> transform.scale.x;
      transNode[7] >> transform.scale.y;
      transNode[8] >> transform.scale.z;

      result->setTransform(transform);
    }

    if (objectNode.has_child("components")) {
      for (auto&& compNode : objectNode["components"]) {
        std::string type;
        compNode["type"] >> type;

        auto parser = assetDatabase()->getComponentParser(type);
        if (!parser) {
          std::cerr << "Unknown component with type: " << type << std::endl;
          continue;
        }

        auto comp = parser->create();
        result->addComponent(comp);

        // Map the component to the parser and node, we'll parse the components
        // after we've created all game objects and their components
        compToParserAndNode_.emplace(comp, std::make_pair(parser, compNode));
      }
    }

    // Recurse into each object and add them.
    if (objectNode.has_child("objects")) {
      for (auto&& objNode : objectNode["objects"]) {
        if (objNode.has_child("scene")) {
          std::string name;
          objNode["name"] >> name;

          // For scene nodes we ignore all other properties and child objects
          // except for the active and scene properties.
          std::string sceneName;
          objNode["scene"] >> sceneName;

          bool active = true;
          if (objNode.has_child("active")) {
            objNode["active"] >> active;
          }
          // Any scenes we're depending on should also be loaded. This is
          // recursive so we can end up loading many scenes deep. If we have
          // circular scene dependencies we will run out of memory or crash due
          // to infinite recursion.
          auto scene = co_await db.loadAssetAsync<Scene>(sceneName);
          scene->setActive(active);

          // Object name may differ from original scene name
          scene->setName(name);

          // Because we're lazy we don't have to worry about attach running
          // until after all objects are added.
          result->addChild(scene);
        } else {
          std::string idStr;
          objNode["id"] >> idStr;

          auto child = GameObject::create(Guid(idStr), true /* lazy attach */);

          loadObject(db, objNode, child);
          result->addChild(std::move(child));
        }
      }
    }

    co_return;
  }

  void postLoad(GameObjectPtr const& root) {
    // Parse the nodes, loading their data. We do this after creating all game
    // objects and components because a component might reference a game
    // object in the scene and we need to be able to resolve that.
    for (auto&& compParserNode : compToParserAndNode_) {
      auto const& [comp, pair] = compParserNode;
      auto const& [parser, node] = pair;

      parser->parse(comp, node, root);
    }
  }

 private:
  std::unordered_map<
      ComponentPtr,
      std::pair<IComponentParserPtr, ryml::ConstNodeRef>>
      compToParserAndNode_;

  size_t tmpCount_{0};
};
} // namespace

auto SceneLoader::loadAssetAsync(AssetDatabase& db, std::vector<char> data)
    -> concurrencpp::result<IAssetPtr> {
  // Create the scene with lazy attachment, this allows us to trigger the
  // attachment when we've finished loading and added it to the root object.
  auto tree = ryml::parse_in_place(data.data());
  auto root = tree.rootref();

  std::string idStr;
  root["id"] >> idStr;

  auto scene = Scene::create(Guid(idStr), true /* lazy attach */);

  Loader loader;
  co_await loader.loadObject(db, root, scene);
  loader.postLoad(scene);

  std::static_pointer_cast<Scene>(scene)->onLoadCompleted();
  co_return scene;
}
} // namespace ewok
