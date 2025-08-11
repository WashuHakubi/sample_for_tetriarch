/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <shared/content_db.h>

#include "../../include/server/systems/spawn_system.h"
#include "../../include/server/random.h"

ewok::server::SpawnSystem::SpawnSystem(shared::IContentDbPtr contentDb)
  : contentDb_(std::move(contentDb)) {
  msgHandle_ = shared::subscribeMessage([this](MobKilled const& msg) { this->onMobKilled(msg); });

  auto spawnDefs = contentDb_->getAllInScope<design_data::SpawnDef>(shared::ContentScope::Map);
  spawns_.reserve(spawnDefs.size());

  uint32_t id{};
  for (auto&& def : spawnDefs) {
    // Every spawn needs some mobs spawned.
    auto& spawn = spawns_.emplace_back(id++, def.resolve(*contentDb_), *contentDb_);
    auto spawnDef = spawn.spawnDef;

    // Spawn a number of mobs between min and max spawn count. This avoids spawns taking a bunch of time after startup.
    auto spawnCount = Random::next(spawnDef->minSpawnCount, spawnDef->maxSpawnCount);
    spawnMobs(spawn, spawnCount);
  }
}

void ewok::server::SpawnSystem::update(float dt) {
  // Only walk spawns we need.
  for (size_t i = 0; i < needsSpawns_.size();) {
    auto& spawn = spawns_[needsSpawns_[i]];
    assert(spawn.id == needsSpawns_[i]);

    if (spawn.needsSpawn && spawn.curSpawnCount >= spawn.minSpawnCount) {
      spawn.needsSpawn = false;
      // Remove ourselves by swapping with the last item (which could be our self)
      needsSpawns_[i] = needsSpawns_.back();
      // Remove the last item.
      needsSpawns_.pop_back();

      // Repeat the check for the new current item.
      continue;
    }

    if (spawn.needsSpawn) [[unlikely]]{
      spawn.spawnTime -= dt;

      if (spawn.spawnTime <= 0.0f) {
        // The countdown has expired, so spawn some mobs!
        auto spawnCount = Random::next(spawn.spawnDef->minSpawnAtOnce, spawn.spawnDef->maxSpawnAtOnce);
        spawnMobs(spawn, spawnCount);

        // This ensures we don't do large spawns repeatedly. If we've met the amount needed for minSpawnCount then the
        // next time we're through the loop the first if condition will remove us.
        spawn.spawnTime = spawn.spawnDef->timeBetweenSpawns;
      }
    }

    // Move to the next item
    ++i;
  }
}

void ewok::server::SpawnSystem::spawnMobs(std::vector<SpawnData>::value_type& spawn, uint32_t spawnCount) {
  // Make sure we don't exceed the max spawn count
  auto const nextSpawnCount = std::min(spawn.curSpawnCount + spawnCount, spawn.spawnDef->maxSpawnCount);

  // and recompute the number of mobs we need to spawn
  spawnCount = nextSpawnCount - spawn.curSpawnCount;
  LOG(INFO) << "Spawning " << spawnCount << " for spawn:" << spawn.id << " " << spawn.spawnDef->id;

  // Get the sum of the total probabilities for the mob spawns.
  const auto probSum = std::accumulate(
      spawn.spawnDef->spawnProbabilities.begin(),
      spawn.spawnDef->spawnProbabilities.end(),
      0.0f,
      [](auto lhs, auto const& rhs) { return lhs + rhs.second; });

  for (auto j = 0u; j < spawnCount; ++j) {
    // Pick a random position in the spawn positions to spawn the mob at.
    // A smarter method would check if the spawn position is "occupied"
    auto const spawnPosIdx = Random::next(0, spawn.spawnDef->spawnPositions.size() - 1);
    auto const& spawnPos = spawn.spawnDef->spawnPositions[spawnPosIdx];

    // Roll a probability for the mob spawn.
    const auto chosenProb = Random::nextReal(0, probSum);
    auto curProb = 0.0f;

    // Walk each mob in the stack of probabilities, adding their probability to curProb. Then if the chosen
    // probability is less than or equal to the sum we have the mob which should be selected.
    // For example, given: [orc:1,goblin:2] and a probability of 1.5. First we add the orc's probability to
    // curProb, curProb = 1, 1.5 is not less than or equal to 1, so we move to the next mob
    // curProb = 3, 1.5 is less than 3 so we spawn the goblin.
    for (auto&& [mob, prob] : spawn.spawnDef->spawnProbabilities) {
      curProb += prob;
      if (chosenProb <= curProb) {
        LOG(INFO) << "Requesting spawn of " << mob.guid() << " for spawn:" << spawn.id << " "
            << spawn.spawnDef->id
            << " at "
            << spawnPos;
        shared::sendMessage(SpawnMobRequest{mob.resolve(*contentDb_), spawn.id, spawnPos, glm::quat{}});
        break;
      }
    }
  }

  // Update our spawn count to the number of mobs spawned.
  spawn.curSpawnCount = nextSpawnCount;
}

void ewok::server::SpawnSystem::onMobKilled(MobKilled const& mobKilled) {
  auto& spawn = spawns_[mobKilled.spawnId];
  [[maybe_unused]] const auto lastCount = spawn.curSpawnCount--;
  assert(spawn.curSpawnCount < lastCount);

  if (!spawn.needsSpawn && spawn.curSpawnCount < spawn.minSpawnCount) {
    // If we didn't need to spawn previously, but now we do, mark us as needing spawns and add us to the list to check.
    spawn.needsSpawn = true;
    spawn.spawnTime = spawn.spawnDef->timeBetweenSpawns;
    needsSpawns_.push_back(spawn.id);
  }
}