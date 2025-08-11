/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <catch2/catch_all.hpp>

#include <server/systems/spawn_messages.h>
#include <server/systems/mob_system.h>

#include "fake_content_db.h"


using namespace ewok;

TEST_CASE("Can spawn mob on request") {
  std::shared_ptr<FakeContentDb> db = std::make_shared<FakeContentDb>();
  populateDb(*db);

  server::MobSystem mobSystem(db);
  auto& spawns = server::MobSystemDebug::debugGetSpawns(mobSystem);

  auto mobs = db->getAllInScope<shared::design_data::MobDef>(shared::ContentScope::Global);

  // Nothing should be spawned.
  REQUIRE(spawns.size() == 0);

  int spawned{};
  auto handle = shared::subscribeMessage(
      [&](server::MobSpawned const& msg) {
        ++spawned;
      });

  REQUIRE(spawned == 0);

  shared::sendMessage(
      server::SpawnMobRequest{
          mobs[0].resolve(*db),
          1,
          glm::vec3{},
          glm::quat{}
      });

  REQUIRE(spawned == 1);
  REQUIRE(spawns.size() == 1);
}

TEST_CASE("Can damage and kill mobs") {
  std::shared_ptr<FakeContentDb> db = std::make_shared<FakeContentDb>();
  populateDb(*db);

  int deathCount{};
  auto handle = shared::subscribeMessage(
      [&](server::MobKilled const&) {
        ++deathCount;
      });

  server::MobSystem mobSystem(db);
  auto& spawns = server::MobSystemDebug::debugGetSpawns(mobSystem);

  auto mobs = db->getAllInScope<shared::design_data::MobDef>(shared::ContentScope::Global);
  shared::sendMessage(
      server::SpawnMobRequest{
          mobs[0].resolve(*db),
          1,
          glm::vec3{},
          glm::quat{}
      });

  REQUIRE(spawns.size() == 1);

  auto damageAmount = spawns[0].curHealth;
  REQUIRE(spawns[0].curHealth > 0);
  damageAmount /= 2;
  auto nextHealth = spawns[0].curHealth - damageAmount;

  // Deal half the life in damage
  shared::sendMessage(
      server::MobDamageRequest{
          0,
          damageAmount,
      });

  REQUIRE(spawns[0].curHealth == nextHealth);
  REQUIRE(!spawns[0].dead);
  REQUIRE(deathCount == 0);

  // Kill the mob
  shared::sendMessage(
      server::MobDamageRequest{
          0,
          nextHealth,
      });

  REQUIRE(spawns[0].curHealth == 0);
  REQUIRE(spawns[0].dead);
  REQUIRE(deathCount == 1);
}