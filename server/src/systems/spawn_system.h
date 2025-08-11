/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <shared/message_dispatch.h>

#include "systems/spawn_messages.h"
#include "systems/mob_messages.h"

namespace ewok::server {
struct SpawnData {
  explicit SpawnData(uint32_t id, design_data::SpawnDefPtr spawnDef)
    : id(id),
      spawnDef(std::move(spawnDef)),
      minSpawnCount(this->spawnDef->minSpawnCount) {
  }

  uint32_t id;

  // Pointer to the design data for this spawn
  design_data::SpawnDefPtr spawnDef{};

  // Copy of the spawn count value, since we use this frequently we cache this here.
  uint32_t minSpawnCount{};

  // Number of mobs currently spawned.
  uint32_t curSpawnCount{};

  // If true, we need to spawn some mobs
  bool needsSpawn{};

  // Number of seconds until the spawn is triggered.
  float spawnTime{};
};

// Example system for handling spawning, this system depends on MobSystem already existing.
class SpawnSystem {
public:
  SpawnSystem();

  void update(float dt);

private:
  friend struct SpawnSystemDebug;

  static void spawnMobs(std::vector<SpawnData>::value_type& spawn, uint32_t spawnCount);

  void onMobKilled(MobKilled const& mobKilled);

  std::vector<SpawnData> spawns_{};
  std::vector<uint32_t> needsSpawns_{};
  shared::MsgDispatchHandle msgHandle_;
};

struct SpawnSystemDebug {
  static auto debugGetSpawns(SpawnSystem const& system) -> std::vector<SpawnData> const& {
    return system.spawns_;
  }

  static auto debugGetNeedsSpawn(SpawnSystem const& system) -> std::vector<uint32_t> const& {
    return system.needsSpawns_;
  }
};
}