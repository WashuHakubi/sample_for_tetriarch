/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "archetypes.h"

ew::Entity ew::Archetypes::createFromSet(ComponentSet const& componentTypes) {
  assert(!traversing_ && "Cannot mutate archetypes while executing an entity query.");
  auto archetype = getOrCreateArchetype(componentTypes);
  size_t index = archetype->allocate();
  const auto entities = archetype->getComponents<Entity>();
  auto desc = std::make_unique<EntityDescriptor>(archetype, index);
  const auto entity = Entity{desc.get()};

  entities[index] = entity;
  entities_.emplace_back(std::move(desc));

  // ReSharper disable once CppDFALocalValueEscapesFunction
  return entity;
}

void ew::Archetypes::destroy(Entity entity) {
  assert(!traversing_ && "Cannot mutate archetypes while executing an entity query.");
  const auto archetype = entity.descriptor->archetype->shared_from_this();
  assert(std::ranges::find(archetypes_, archetype) != archetypes_.end());

  archetype->release(entity.descriptor->id);
  const auto end =
      std::ranges::remove_if(entities_, [entity](auto const& item) { return item.get() == entity.descriptor; }).begin();
  entities_.erase(end, entities_.end());
}

void ew::Archetypes::beginTraversal() const {
  assert(!traversing_);
  traversing_ = true;
}

void ew::Archetypes::endTraversal() const {
  assert(traversing_);
  traversing_ = false;
}

void ew::Archetypes::copy(ArchetypePtr const& from, ArchetypePtr const& to, const size_t fromId, const size_t toId) {
  for (auto&& component : from->componentData_) {
    auto it = to->componentData_.find(component.first);
    if (it == to->componentData_.end())
      continue;

    it->second->setFrom(component.second, fromId, toId);
  }
}

ew::ArchetypePtr ew::Archetypes::getOrCreateArchetype(ComponentSet const& componentTypes) {
  for (auto&& archetype : archetypes_) {
    if (archetype->components_ == componentTypes) {
      return archetype;
    }
  }

  // Create a copy of the component types and ensure Entity is in it
  auto newComponentTypes = componentTypes;
  set(newComponentTypes, getComponentId<Entity>());
  std::unordered_map<ComponentId, ArchetypeStoragePtr> componentData;

  // Create the storage containers for the components
  forEach(newComponentTypes, [this, &componentData](auto id) {
    if (auto it = componentIdToStorage_.find(id); it == componentIdToStorage_.end()) {
      assert(false && "Need to register all component types before allocating _archetypes.");
    } else {
      componentData.emplace(id, it->second());
    }
  });

  auto result = std::make_shared<Archetype>(std::move(componentData), newComponentTypes);
  archetypes_.emplace_back(result);
  return result;
}