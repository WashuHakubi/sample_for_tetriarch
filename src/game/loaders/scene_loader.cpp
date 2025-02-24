/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include <iostream>

#include "engine/asset_database.h"
#include "engine/component.h"
#include "engine/math.h"
#include "engine/object_database.h"
#include "engine/reflection/reflection.h"
#include "game/loaders/scene_loader.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

namespace ewok {
namespace {

template <class T>
void readField(
    InstancePtr const& instance,
    FieldPtr field,
    ryml::ConstNodeRef componentNode) {
  T value;
  componentNode[field->name().c_str()] >> value;
  field->setValue<T>(instance, value);
}

template <class T, size_t N>
void readVector(
    InstancePtr const& instance,
    FieldPtr field,
    ryml::ConstNodeRef componentNode) {
  VectorN<T, N> vec;
  auto node = componentNode[field->name().c_str()];
  for (size_t i = 0; i < N; ++i) {
    node[i] >> vec.v[i];
  }
  field->setValue<VectorN<T, N>>(instance, vec);
}

void readQuaternion(
    InstancePtr const& instance,
    FieldPtr field,
    ryml::ConstNodeRef componentNode) {
  Quat q;
  auto node = componentNode[field->name().c_str()];
  node[0] >> q.x;
  node[1] >> q.y;
  node[2] >> q.z;
  node[3] >> q.w;
  field->setValue<Quat>(instance, q);
}

void readObjectField(
    InstancePtr const& instance,
    FieldPtr field,
    ryml::ConstNodeRef componentNode) {
  std::string value;
  componentNode[field->name().c_str()] >> value;
  auto p = objectDatabase()->find(Guid(value));
  field->setValue<GameObjectHandle>(instance, GameObjectHandle{p});
}

using ReadFieldMethod =
    void (*)(InstancePtr const&, FieldPtr, ryml::ConstNodeRef);

void readComponentFields(
    InstancePtr const& instance,
    std::type_index type,
    ryml::ConstNodeRef componentNode) {
  static const std::unordered_map<std::type_index, ReadFieldMethod> s_readers =
      {
          {typeid(int8_t), readField<int8_t>},
          {typeid(int16_t), readField<int16_t>},
          {typeid(int32_t), readField<int32_t>},
          {typeid(int64_t), readField<int64_t>},

          {typeid(uint8_t), readField<uint8_t>},
          {typeid(uint16_t), readField<uint16_t>},
          {typeid(uint32_t), readField<uint32_t>},
          {typeid(uint64_t), readField<uint64_t>},

          {typeid(float), readField<float>},
          {typeid(double), readField<double>},

          {typeid(Vec2), readVector<float, 2>},
          {typeid(Vec3), readVector<float, 3>},
          {typeid(Vec4), readVector<float, 4>},

          {typeid(Int8Vec2), readVector<int8_t, 2>},
          {typeid(Int8Vec3), readVector<int8_t, 3>},
          {typeid(Int8Vec4), readVector<int8_t, 4>},

          {typeid(UInt8Vec2), readVector<uint8_t, 2>},
          {typeid(UInt8Vec3), readVector<uint8_t, 3>},
          {typeid(UInt8Vec4), readVector<uint8_t, 4>},

          {typeid(Quat), readQuaternion},

          {typeid(bool), readField<bool>},
          {typeid(std::string), readField<std::string>},
          {typeid(GameObjectHandle), readObjectField},

      };

  auto classPtr = Reflection::class_(type);
  if (!classPtr) {
    std::cerr << "Unable to read component, no reflection data exists.";
    return;
  }

  for (auto&& field : classPtr->fields()) {
    // Only load nodes we have fields for
    if (componentNode.has_child(field->name().c_str())) {
      if (auto it = s_readers.find(field->type()); it != s_readers.end()) {
        it->second(instance, field, componentNode);
      } else {
        auto childNode = componentNode[field->name().c_str()];
        if (childNode.is_map()) {
          // Recurse into child nodes and try loading them.
          auto childPtr = field->valuePtr(instance);
          readComponentFields(childPtr, field->type(), childNode);
        }
      }
    }
  }
}

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
      transNode[0] >> transform.position.v[0];
      transNode[1] >> transform.position.v[1];
      transNode[2] >> transform.position.v[2];

      float roll;
      float pitch;
      float yaw;
      transNode[3] >> roll;
      transNode[4] >> pitch;
      transNode[5] >> yaw;
      auto eulerAngles = asEuler(roll, pitch, yaw);
      transform.rotation = fromEuler(eulerAngles);

      transNode[6] >> transform.scale.v[0];
      transNode[7] >> transform.scale.v[1];
      transNode[8] >> transform.scale.v[2];

      result->setTransform(transform);
    }

    if (objectNode.has_child("components")) {
      for (auto&& compNode : objectNode["components"]) {
        std::string type;
        compNode["type"] >> type;

        auto classPtr = Reflection::class_(type);
        if (!classPtr) {
          std::cerr << "Unknown component with type: " << type << std::endl;
          continue;
        }

        auto comp = std::static_pointer_cast<ComponentBase>(classPtr->create());
        if (!comp) {
          std::cerr << "Unable to create " << classPtr->name()
                    << " type does not have a default constructor.";
          continue;
        }
        result->addComponent(comp);

        // Map the component to the parser and node, we'll parse the components
        // after we've created all game objects and their components
        compToNode_.emplace(comp, compNode);
      }
    }

    // Recurse into each object and add them.
    if (objectNode.has_child("objects")) {
      for (auto&& objNode : objectNode["objects"]) {
        if (objNode.has_child("scene")) {
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
    for (auto&& compNode : compToNode_) {
      auto const& [comp, node] = compNode;
      readComponentFields(
          {comp.get(), typeid(ComponentBase)}, comp->getComponentType(), node);
    }
  }

 private:
  std::unordered_map<ComponentPtr, ryml::ConstNodeRef> compToNode_;

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
