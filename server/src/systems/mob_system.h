/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <shared/message_dispatch.h>

#include "systems/mob_messages.h"
#include "systems/spawn_messages.h"

namespace ewok::server {
struct MobData {
  MobData(uint32_t spawnId, shared::design_data::MobDefPtr mobDef, glm::vec3 position, glm::quat rotation)
    : spawnId(spawnId),
      mobDef(std::move(mobDef)),
      position(position),
      rotation(rotation) {
    curHealth = maxHealth = static_cast<int32_t>(mobDef->health);
  }

  bool dead{false};

  uint32_t spawnId;

  shared::design_data::MobDefPtr mobDef;

  glm::vec3 position{};

  glm::quat rotation{};

  int32_t maxHealth{};

  int32_t curHealth{};
};

class MobSystem {
public:
  MobSystem();

private:
  void onSpawnMobRequest(SpawnMobRequest const& req);

  void onDamageMobRequest(MobDamageRequest const& req);

  std::vector<MobData> mobs_;
  std::vector<uint32_t> freeIds_;
  shared::MsgDispatchHandle spawnMobRequest_;
  shared::MsgDispatchHandle damageMobRequest_;
};
} // namespace ewok::server
