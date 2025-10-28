/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "entity.h"

#include <cassert>

ew::ComponentId ew::detail::nextComponentId() {
  static ComponentId id = 0;
  assert(id < MAX_COMPONENT_ID);
  return id++;
}

bool ew::test(ComponentSet const& set, const ComponentId id) {
  return set.test(id);
}

bool ew::allOf(ComponentSet const& set, ComponentSet const& expected) {
  return (set & expected) == expected;
}

bool ew::noneOf(ComponentSet const& set, ComponentSet const& expected) {
  return (set & expected) == 0;
}

void ew::set(ComponentSet& set, const ComponentId id) {
  set.set(id);
}

void ew::set(ComponentSet& set, ComponentSet const& expected) {
  set |= expected;
}

void ew::clear(ComponentSet& set, const ComponentId id) {
  set.set(id, false);
}

void ew::forEach(ComponentSet const& set, std::function<void(ComponentId id)> const& fn) {
  for (auto i = 0u; i < set.size(); i++) {
    if (set.test(i)) {
      fn(ComponentId{i});
    }
  }
}

ew::ComponentSet ew::fromIds(std::initializer_list<ComponentId> const& ids) {
  ComponentSet set;
  for (auto&& id : ids) {
    set.set(id, true);
  }
  return set;
}