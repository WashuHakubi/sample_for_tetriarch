/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <server/systems/spawn_system.h>

#include <catch2/catch_all.hpp>
#include "fake_content_db.h"

using namespace ewok;

TEST_CASE("Spawner will attempt to spawn on construction") {
  std::shared_ptr<FakeContentDb> db = std::make_shared<FakeContentDb>();
  populateDb(*db);

  int spawnsRequestedCount{};
  auto handler = shared::subscribeMessage(
      [&spawnsRequestedCount](server::SpawnMobRequest const&) {
        ++spawnsRequestedCount;
      });
  REQUIRE(spawnsRequestedCount == 0);

  server::SpawnSystem spawnSystem(db);
  // We should have requested 4 mobs to spawn.
  REQUIRE(spawnsRequestedCount == 4);
}

TEST_CASE("Killing mobs below min spawn will trigger mob spawning") {
  std::shared_ptr<FakeContentDb> db = std::make_shared<FakeContentDb>();
  populateDb(*db);

  uint32_t mobId{};
  auto handler = shared::subscribeMessage(
      [&](server::SpawnMobRequest const& req) {
        // Pretend to spawn the mob.
        shared::sendMessage(
            server::MobSpawned{
                ++mobId,
                req.spawnId,
                req.mob,
                req.mob->health,
                req.position,
                req.rotation});
      });

  server::SpawnSystem spawnSystem(db);
  REQUIRE(mobId == 4);

  // This should have no effect.
  spawnSystem.update(4);

  auto& debugSpawns = server::SpawnSystemDebug::debugGetSpawns(spawnSystem);
  REQUIRE(debugSpawns.size() == 1);
  REQUIRE(debugSpawns[0].curSpawnCount == 4);
  REQUIRE(!debugSpawns[0].needsSpawn);

  // Kill one mob.
  shared::sendMessage(server::MobKilled{0, debugSpawns[0].id});

  // We should need a mob to spawn
  REQUIRE(debugSpawns[0].curSpawnCount == 3);
  REQUIRE(debugSpawns[0].needsSpawn);

  // Pretend we spent two seconds, which matches our spawn time duration
  spawnSystem.update(2);

  // We should have spawned a new mob.
  REQUIRE(mobId == 5);
  REQUIRE(debugSpawns[0].curSpawnCount == 4);

  // However we have not deflagged from needing a spawn
  REQUIRE(debugSpawns[0].needsSpawn);

  spawnSystem.update(0);

  // Now we should no longer be requesting a spawn.
  REQUIRE(!debugSpawns[0].needsSpawn);
}
