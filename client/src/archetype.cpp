/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "archetype.h"

#include <ranges>

ew::Archetype::Archetype(std::unordered_map<ComponentId, ArchetypeStoragePtr> components, ComponentSet componentTypes)
    : componentData_(std::move(components))
    , components_(std::move(componentTypes)) {
  set(components_, getComponentId<Entity>());
}

size_t ew::Archetype::allocate() {
  if (free_.empty()) {
    const size_t id = nextId_++;

    for (auto& store : componentData_ | std::views::values) {
      const auto index = store->alloc();
      assert(index == id);
    }

    return id;
  }

  const size_t id = *free_.begin();
  free_.erase(id);
  return id;
}

void ew::Archetype::release(size_t id) {
  assert(!std::ranges::contains(free_, id));
  free_.emplace(id);
}

bool ew::Archetype::match(ComponentSet const& with, ComponentSet const& without) {
  if (!allOf(components_, with)) {
    return false;
  }

  if (!noneOf(components_, without)) {
    return false;
  }

  return true;
}