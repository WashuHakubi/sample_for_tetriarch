/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "archetype.h"
#include "entity_query.h"

namespace ew {
class Archetypes : IArchetypeTraversable {
 public:
  Archetypes() { registerComponent<Entity>(); }

  template <class T>
  ComponentId registerComponent();

  template <class... Ts>
  [[nodiscard]] Entity create();

  template <class... Ts>
  [[nodiscard]] Entity create(Ts&&... ts);

  [[nodiscard]] Entity createFromSet(ComponentSet const& componentTypes);

  void destroy(Entity entity);

  template <class... Ts>
  void addComponents(Entity entity, Ts&&... ts);

  template <class... Ts>
  void removeComponents(Entity entity);

  [[nodiscard]] EntityQuery query() const { return EntityQuery{this}; }

 private:
  friend class EntityQuery;

  void beginTraversal() const override;

  void endTraversal() const override;

  std::vector<ArchetypePtr> const& archetypes() const override { return archetypes_; }

  static void copy(ArchetypePtr const& from, ArchetypePtr const& to, size_t fromId, size_t toId);

  template <class T>
  static void assign(ArchetypePtr const& ptr, size_t index, T&& v);

  template <class... Ts>
  ArchetypePtr getOrCreateArchetype();

  ArchetypePtr getOrCreateArchetype(ComponentSet const& componentTypes);

  std::unordered_map<ComponentId, std::function<ArchetypeStoragePtr()>> componentIdToStorage_;
  std::vector<ArchetypePtr> archetypes_;
  std::vector<std::unique_ptr<EntityDescriptor>> entities_;
  mutable bool traversing_{false};
};

template <class T>
ComponentId Archetypes::registerComponent() {
  return componentIdToStorage_.emplace(getComponentId<T>(), [] { return std::make_unique<ArchetypeStorage<T>>(); })
      .first->first;
}

template <class... Ts>
Entity Archetypes::create() {
  assert(!traversing_ && "Cannot mutate archetypes while executing an entity query.");
  return createFromSet(fromIds({registerComponent<Ts>()...}));
}

template <class... Ts>
Entity Archetypes::create(Ts&&... ts) {
  assert(!traversing_ && "Cannot mutate archetypes while executing an entity query.");
  auto eid = createFromSet(fromIds({registerComponent<Ts>()...}));
  addComponents(eid, std::forward<Ts>(ts)...);
  return eid;
}

template <class... Ts>
void Archetypes::addComponents(const Entity entity, Ts&&... ts) {
  assert(!traversing_ && "Cannot mutate archetypes while executing an entity query.");
  auto archetype = entity.descriptor->archetype->shared_from_this();
  assert(std::ranges::find(archetypes_, archetype) != archetypes_.end());

  auto next = archetype->components_;
  (set(next, getComponentId<Ts>()), ...);

  auto curId = entity.descriptor->id;

  auto nextArchetype = getOrCreateArchetype(next);
  if (nextArchetype == archetype) {
    // Update the component values
    (assign(archetype, curId, std::forward<Ts>(ts)), ...);
    return;
  }

  size_t nextId = nextArchetype->allocate();
  copy(archetype, nextArchetype, curId, nextId);

  (assign(nextArchetype, nextId, ts), ...);

  archetype->release(curId);
  entity.descriptor->archetype = nextArchetype;
  entity.descriptor->id = nextId;
}

template <class... Ts>
void Archetypes::removeComponents(const Entity entity) {
  assert(!traversing_ && "Cannot mutate archetypes while executing an entity query.");
  const auto archetype = entity.descriptor->archetype->shared_from_this();
  assert(std::ranges::find(archetypes_, archetype) != archetypes_.end());

  auto next = archetype->components_;
  (clear(next, getComponentId<Ts>()), ...);

  const auto curId = entity.descriptor->id;

  const auto nextArchetype = getOrCreateArchetype(next);
  if (nextArchetype == archetype)
    return;

  const size_t nextId = nextArchetype->allocate();
  copy(archetype, nextArchetype, curId, nextId);

  archetype->release(curId);
  entity.descriptor->archetype = nextArchetype;
  entity.descriptor->id = nextId;
}

template <class T>
void Archetypes::assign(ArchetypePtr const& ptr, size_t index, T&& v) {
  ptr->getComponents<std::decay_t<T>>()[index] = std::forward<T>(v);
}

template <class... Ts>
ArchetypePtr Archetypes::getOrCreateArchetype() {
  ComponentSet componentIds;
  (set(componentIds, getComponentId<Ts>()), ...);

  return getOrCreateArchetype(componentIds);
}
} // namespace ew
