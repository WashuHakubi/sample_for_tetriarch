/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <shared/message_dispatch.h>

#include "server/systems/mob_messages.h"
#include "spawn_messages.h"

namespace ewok::server {
struct MobData {
  MobData(uint32_t spawnId, shared::design_data::MobDef const* mobDef, glm::vec3 position, glm::quat rotation)
    : spawnId(spawnId),
      mobDef(mobDef),
      position(position),
      rotation(rotation) {
    curHealth = maxHealth = static_cast<int32_t>(this->mobDef->health);
  }

  bool dead{false};

  uint32_t spawnId;

  shared::design_data::MobDef const* mobDef;

  glm::vec3 position{};

  glm::quat rotation{};

  int32_t maxHealth{};

  int32_t curHealth{};
};

class MobSystem {
public:
  MobSystem(shared::IContentDbPtr contentDb);

  auto debugGetMobs() -> std::vector<MobData> const& {
    return mobs_;
  }

private:
  friend struct MobSystemDebug;

  void onSpawnMobRequest(SpawnMobRequest const& req);

  void onDamageMobRequest(MobDamageRequest const& req);

  shared::IContentDbPtr contentDb_;
  std::vector<MobData> mobs_;
  std::vector<uint32_t> freeIds_;
  shared::MsgDispatchHandle spawnMobRequest_;
  shared::MsgDispatchHandle damageMobRequest_;
};

struct MobSystemDebug {
  static auto debugGetSpawns(MobSystem const& system) -> std::vector<MobData> const& {
    return system.mobs_;
  }
};
} // namespace ewok::server
