/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_set>

namespace ew {
class Archetype;

using ComponentId = uint32_t;
using ComponentSet = std::unordered_set<ComponentId>;

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
  EntityDescriptor(std::shared_ptr<Archetype> archetype, size_t index) : archetype(std::move(archetype)), id(index) {}

  std::shared_ptr<Archetype> archetype;
  size_t id;
};

struct Entity {
  EntityDescriptor* descriptor;
};
} // namespace ew