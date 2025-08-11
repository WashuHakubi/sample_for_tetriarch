/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <catch2/catch_all.hpp>

#include <server/systems/mob_system.h>
#include <server/systems/spawn_system.h>

#include "fake_content_db.h"

using namespace ewok;

TEST_CASE("Mob/Spawn Integration") {
  auto db = std::make_shared<ewok::FakeContentDb>();
  populateDb(*db);

  server::MobSystem mobSystem(db);
  auto& mobs = server::MobSystemDebug::debugGetSpawns(mobSystem);
  // No mobs should have spawned
  REQUIRE(mobs.size() == 0);

  server::SpawnSystem spawnSystem(db);
  auto& spawns = server::SpawnSystemDebug::debugGetSpawns(spawnSystem);

  // After the system is constructed we should have 4 mobs
  REQUIRE(spawns.size() == 1);
  REQUIRE(!spawns[0].needsSpawn);
  REQUIRE(mobs.size() == 4);

  // Damaging a mob should not cause it to spawn.
  shared::sendMessage(
      server::MobDamageRequest{
          0,
          mobs[0].curHealth / 2
      });

  REQUIRE(!mobs[0].dead);
  REQUIRE(!spawns[0].needsSpawn);

  spawnSystem.update(2);

  REQUIRE(!mobs[0].dead);
  REQUIRE(!spawns[0].needsSpawn);

  // Kill the mob
  shared::sendMessage(
      server::MobDamageRequest{
          0,
          mobs[0].curHealth
      });

  REQUIRE(mobs[0].dead);
  REQUIRE(mobs.size() == 4);
  REQUIRE(spawns[0].needsSpawn);

  spawnSystem.update(2);

  REQUIRE(!mobs[0].dead);
  REQUIRE(mobs.size() == 4);
  REQUIRE(spawns[0].needsSpawn);

  spawnSystem.update(2);

  REQUIRE(!mobs[0].dead);
  REQUIRE(mobs.size() == 4);
  REQUIRE(!spawns[0].needsSpawn);
}