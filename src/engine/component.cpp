/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "engine/component.h"
#include "engine/reflection/reflection.h"

EWOK_REGISTRATION {
  using namespace ewok;

  Register::class_<ComponentBase>("Component");
}
namespace ewok {} // namespace ewok
