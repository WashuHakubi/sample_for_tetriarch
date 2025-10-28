/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "entity.h"

namespace ew::detail {
ComponentId nextComponentId() {
  static ComponentId id = 0;
  return id++;
}
} // namespace ew::detail