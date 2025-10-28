/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "entity_query.h"
ew::EntityQuery& ew::EntityQuery::with(std::initializer_list<ComponentId> ids) {
  return addComponents(requiredComponents_, ids);
}

ew::EntityQuery& ew::EntityQuery::without(std::initializer_list<ComponentId> ids) {
  return addComponents(withoutComponents_, ids);
}

ew::EntityQuery& ew::EntityQuery::atLeastOneOf(std::initializer_list<ComponentId> ids) {
  return addComponents(atLeastOneComponents_, ids);
}

ew::EntityQuery& ew::EntityQuery::addComponents(ComponentSet& set, std::initializer_list<ComponentId> ids) {
  for (auto&& id : ids) {
    set.emplace(id);
  }
  return *this;
}