/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "entity_query.h"

ew::EntityQuery& ew::EntityQuery::with(ComponentSet const& ids) {
  set(requiredComponents_, ids);
  return *this;
}

ew::EntityQuery& ew::EntityQuery::without(ComponentSet const& ids) {
  set(withoutComponents_, ids);
  return *this;
}
