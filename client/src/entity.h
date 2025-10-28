/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <bitset>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_set>

/// Maximum component ID. Increase this value if you have more than 256 components.
#define MAX_COMPONENT_ID 256

namespace ew {
class Archetype;

using ComponentId = uint32_t;
using ComponentSet = std::bitset<MAX_COMPONENT_ID>;

bool test(ComponentSet const& set, ComponentId id);

bool allOf(ComponentSet const& set, ComponentSet const& expected);

bool noneOf(ComponentSet const& set, ComponentSet const& expected);

void set(ComponentSet& set, ComponentId id);

void set(ComponentSet& set, ComponentSet const& expected);

void clear(ComponentSet& set, ComponentId id);

void forEach(ComponentSet const& set, std::function<void(ComponentId id)> const& fn);

ComponentSet fromIds(std::initializer_list<ComponentId> const& ids);

namespace detail {
ComponentId nextComponentId();
}

// Gets a component
template <class T>
ComponentId getComponentId() {
  static const ComponentId id = detail::nextComponentId();
  return id;
}

struct EntityDescriptor {
  EntityDescriptor(std::shared_ptr<Archetype> archetype, const size_t index)
      : archetype(std::move(archetype))
      , id(index) {}

  std::shared_ptr<Archetype> archetype;
  size_t id;
};

struct Entity {
  EntityDescriptor* descriptor;
};
} // namespace ew