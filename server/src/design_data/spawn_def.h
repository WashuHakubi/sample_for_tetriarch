/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

namespace ewok::server::design_data {
struct SpawnDef : shared::ContentDef {
  // Positions to spawn mobs at
  std::vector<glm::vec3> spawnPositions;

  // List of mobs that can spawn and their probability. The probability of a mob spawning is their probability value
  // divided by the total sum of the probabilities. That is: [orc:1, goblin:2] means orcs have a 1:3 chance while
  // goblins have the remaining 2:3 chances.
  std::vector<std::pair<shared::design_data::MobDefPtr, float>> spawnProbabilities;

  // Minimum number of mobs, if we're below this then spawning can be triggered
  uint32_t minSpawnCount{};

  // Maximum number of mobs, at or above this and spawning stops.
  uint32_t maxSpawnCount{};

  // Minimum number of mobs a spawn cycle and create. Each spawn cycle will spawn between [minSpawnAtOnce, maxSpawnAtOnce] mobs.
  uint32_t minSpawnAtOnce{};

  // Maximum number of mobs a spawn cycle and create. Each spawn cycle will spawn between [minSpawnAtOnce, maxSpawnAtOnce] mobs.
  uint32_t maxSpawnAtOnce{};

  // Seconds between attempting to spawn new mobs
  float timeBetweenSpawns{};

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("id", &SpawnDef::id),
        std::make_pair("positions", &SpawnDef::spawnPositions),
        std::make_pair("probabilities", &SpawnDef::spawnProbabilities),
        std::make_pair("minSpawnCount", &SpawnDef::minSpawnCount),
        std::make_pair("maxSpawnCount", &SpawnDef::maxSpawnCount),
        std::make_pair("minSpawnAtOnce", &SpawnDef::minSpawnAtOnce),
        std::make_pair("maxSpawnAtOnce", &SpawnDef::maxSpawnAtOnce),
        std::make_pair("timeBetweenSpawns", &SpawnDef::timeBetweenSpawns));
  }
};

using SpawnDefPtr = shared::ContentPtr<SpawnDef>;
}