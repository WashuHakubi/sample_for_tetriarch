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
    , components_(std::move(componentTypes)) {}

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

bool ew::Archetype::match(ComponentSet const& with, ComponentSet const& without, ComponentSet const& atLeastOne) {
  if (!std::ranges::all_of(with, [this](auto const& item) { return componentData_.contains(item); })) {
    return false;
  }

  if (std::ranges::any_of(without, [this](auto const& item) { return componentData_.contains(item); })) {
    return false;
  }

  if (atLeastOne.empty() ||
      std::ranges::any_of(atLeastOne, [this](auto const& item) { return componentData_.contains(item); })) {
    return true;
  }

  return false;
}