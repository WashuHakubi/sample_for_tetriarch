/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <cstdint>
#include <shared/design_data/design_data.h>

namespace ewok::server {
struct MobKilled {
  uint32_t id;
  uint32_t spawnId;
};

struct MobDamageRequest {
  uint32_t id;
  int32_t amount;
};

struct MobHealthChanged {
  uint32_t id;
  int32_t curHealth;
};

struct MobSpawned {
  uint32_t id;
  uint32_t spawnId;
  shared::design_data::MobDef const* mob;
  int32_t curHealth;
  glm::vec3 position;
  glm::quat rotation;
};
}