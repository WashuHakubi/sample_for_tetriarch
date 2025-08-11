/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "../design_data/design_data.h"

namespace ewok::server {
struct SpawnMobRequest {
  shared::design_data::MobDefPtr mob;
  uint32_t spawnId;
  glm::vec3 position;
  glm::quat rotation;
};
}