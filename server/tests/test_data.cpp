/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <server/design_data/design_data.h>
#include <test_data.h>

#include <catch2/catch_all.hpp>

namespace ewok {
static std::string_view mobDefStr = R"({
  "id": {
    "g": "b5f8b8ee-d67b-4312-b8cf-944934342004"
  },
  "name": "Wolf",
  "rarity": 0,
  "health": 100,
  "model": "wolf.prefab"
})";

static std::string_view spawnDefStr = R"({
  "id": {
    "g": "c119994e-d152-406c-9aa3-8e3b3a555151"
  },
  "positions": [
    {
      "x": 0.0,
      "y": 0.0,
      "z": 0.0
    },
    {
      "x": 1.0,
      "y": 0.0,
      "z": 0.0
    },
    {
      "x": -1.0,
      "y": 0.0,
      "z": 0.0
    },
    {
      "x": 0.0,
      "y": 0.0,
      "z": 1.0
    },
    {
      "x": 0.0,
      "y": 0.0,
      "z": -1.0
    },
    {
      "x": 1.0,
      "y": 0.0,
      "z": 1.0
    },
    {
      "x": 1.0,
      "y": 0.0,
      "z": -1.0
    }
  ],
  "probabilities": [
    {
      "f": {
        "g": "b5f8b8ee-d67b-4312-b8cf-944934342004"
      },
      "s": 1.0
    }
  ],
  "minSpawnCount": 4,
  "maxSpawnCount": 4,
  "minSpawnAtOnce": 1,
  "maxSpawnAtOnce": 1,
  "timeBetweenSpawns": 2.0
})";

void populateDb(shared::design_data::FakeContentDb& contentDb) {
  auto reader = shared::serialization::createJsonReader(mobDefStr);
  auto mobDef = std::make_shared<shared::design_data::MobDef>();
  [[maybe_unused]] auto r = shared::serialization::deserialize(*reader, *mobDef);
  REQUIRE(r.has_value());

  reader->reset(spawnDefStr);
  auto spawnDef = std::make_shared<server::design_data::SpawnDef>();
  r = shared::serialization::deserialize(*reader, *spawnDef);
  REQUIRE(r.has_value());

  // Mobs are globally defined
  contentDb.registerItem(mobDef, shared::ContentScope::Global);

  // Spawns are per-map
  contentDb.registerItem(spawnDef, shared::ContentScope::Map);
}
}