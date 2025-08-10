/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shared/math.h"
#include "shared/content_db.h"

using namespace ewok;

namespace ewok::server {
class MobDef;
using MobDefPtr = shared::ContentPtr<MobDef>;

class SpawnDef;
using SpawnDefPtr = shared::ContentPtr<SpawnDef>;

enum class MobRarity {
  Normal,
  Magic,
  Rare,
  Unique
};

class MobDef : public shared::ContentDef {
  // Name of the mob
  std::string name;

  // Rarity of the mob
  MobRarity rarity{};

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("id", &MobDef::id),
        std::make_pair("name", &MobDef::name),
        std::make_pair("rarity", &MobDef::rarity));
  }
};

class SpawnDef : public shared::ContentDef {
  // Positions to spawn mobs at
  std::vector<glm::vec3> spawnPositions;

  // List of mobs that can spawn and their probability. The probability of a mob spawning is their probability value
  // divided by the total sum of the probabilities. That is: [orc:1, goblin:2] means orcs have a 1:3 chance while
  // goblins have the remaining 2:3 chances.
  std::vector<std::pair<MobDefPtr, float>> spawnProbabilities;

  // Minimum number of mobs, if we're below this then spawning can be triggered
  uint32_t minSpawnCount{};

  // Maximum number of mobs, at or above this and spawning stops.
  uint32_t maxSpawnCount{};

  // Maximum number of mobs a spawn cycle and create. Each spawn cycle will spawn between [0, maxSpawnAtOnce] mobs.
  uint32_t maxSpawnAtOnce{};

  // Seconds between attempting to spawn new mobs
  float timeBetweenSpawns{};

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("id", &SpawnDef::id),
        std::make_pair("positions", &SpawnDef::spawnPositions),
        std::make_pair("probabilities", &SpawnDef::spawnProbabilities),
        std::make_pair("minSpawnCount", &SpawnDef::minSpawnCount),
        std::make_pair("maxSpawnCount", &SpawnDef::maxSpawnCount),
        std::make_pair("maxSpawnAtOnce", &SpawnDef::maxSpawnAtOnce),
        std::make_pair("timeBetweenSpawns", &SpawnDef::timeBetweenSpawns));
  }
};
}

int main() {
  auto writer = shared::serialization::createJsonWriter();
  server::MobDefPtr ptr{xg::newGuid()};

  auto r = shared::serialization::serialize(*writer, ptr);
  std::cout << writer->data() << std::endl;

  return 0;
}