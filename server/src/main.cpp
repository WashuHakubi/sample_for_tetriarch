/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <random>

#include "shared/math.h"
#include "shared/content_db.h"

using namespace ewok;

namespace ewok::server::design_data {
struct MobDef;
using MobDefPtr = shared::ContentPtr<MobDef>;

struct SpawnDef;
using SpawnDefPtr = shared::ContentPtr<SpawnDef>;

enum class MobRarity {
  Normal,
  Magic,
  Rare,
  Unique
};

struct MobDef : public shared::ContentDef {
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

struct SpawnDef : public shared::ContentDef {
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

  // Minimum number of mobs a spawn cycle and create. Each spawn cycle will spawn between [minSpawnAtOnce, maxSpawnAtOnce] mobs.
  uint32_t minSpawnAtOnce{};

  // Maximum number of mobs a spawn cycle and create. Each spawn cycle will spawn between [minSpawnAtOnce, maxSpawnAtOnce] mobs.
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
        std::make_pair("minSpawnAtOnce", &SpawnDef::minSpawnAtOnce),
        std::make_pair("maxSpawnAtOnce", &SpawnDef::maxSpawnAtOnce),
        std::make_pair("timeBetweenSpawns", &SpawnDef::timeBetweenSpawns));
  }
};
}

namespace ewok::server {
struct Random {
  static uint32_t next(uint32_t min, uint32_t max) {
    return std::uniform_int_distribution{min, max}(s_gen);
  }

  static float nextReal(float min = 0, float max = 1) {
    return std::uniform_real_distribution{min, max}(s_gen);
  }

private:
  static std::mt19937 s_gen;
};

struct SpawnData {
  SpawnData(uint32_t id, design_data::SpawnDefPtr spawnDef)
    : id(id),
      spawnDef(std::move(spawnDef)),
      minSpawnCount(this->spawnDef->minSpawnCount) {
  }

  // Id of this spawn, mobs will have the ID of the thing that spawned them.
  uint32_t id{};

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

// Example system for handling spawning
class SpawnSystem {
public:
  void update(float dt) {
    for (auto&& spawn : spawns_) {
      if (!spawn.needsSpawn && spawn.curSpawnCount < spawn.minSpawnCount) {
        spawn.needsSpawn = true;
        spawn.spawnTime = spawn.spawnDef->timeBetweenSpawns;
        continue;
      }

      if (spawn.needsSpawn && spawn.curSpawnCount > spawn.minSpawnCount) {
        spawn.needsSpawn = false;
        continue;
      }

      if (spawn.needsSpawn) {
        spawn.spawnTime -= dt;

        // If our countdown to the spawn has passed then perform a spawn action.
        if (spawn.spawnTime <= 0.0f) {
          auto spawnCount = Random::next(
              spawn.spawnDef->minSpawnAtOnce,
              spawn.spawnDef->maxSpawnAtOnce);

          // Make sure we don't exceed the max spawn count
          auto nextSpawnCount = std::max(spawn.curSpawnCount + spawnCount, spawn.spawnDef->maxSpawnCount);

          // and recompute the number of mobs we need to spawn
          spawnCount = nextSpawnCount - spawn.curSpawnCount;

          // Get the sum of the total probabilities for the mob spawns.
          auto probSum = std::accumulate(
              spawn.spawnDef->spawnProbabilities.begin(),
              spawn.spawnDef->spawnProbabilities.end(),
              0.0f,
              [](auto lhs, auto const& rhs) { return lhs + rhs.second; });

          for (auto i = 0u; i < spawnCount; ++i) {
            // Roll a probability for the mob spawn.
            auto chosenProb = Random::nextReal(0, probSum);
            auto curProb = 0.0f;

            // Walk each mob in the stack of probabilities, adding their probability to curProb. Then if the chosen
            // probability is less than or equal to the sum we have the mob which should be selected.
            // For example, given: [orc:1,goblin:2] and a probability of 1.5. First we add the orc's probability to
            // curProb, curProb = 1, 1.5 is not less than or equal to 1, so we move to the next mob
            // curProb = 3, 1.5 is less than 3 so we spawn the goblin.
            for (auto&& [mob, prob] : spawn.spawnDef->spawnProbabilities) {
              curProb += prob;
              if (chosenProb <= curProb) {
                // TODO: Spawn mob
                break;
              }
            }
          }
          spawn.curSpawnCount = nextSpawnCount;
        }
      }
    }
  }

private:
  std::vector<SpawnData> spawns_{};
};
}

struct FakeContentDb : shared::IContentDb {
  template <class T>
    requires(std::is_base_of_v<shared::ContentDef, T>)
  void registerItem(std::shared_ptr<T> p) {
    auto id = p->id;
    db_.emplace(id, p);
  }

  std::shared_ptr<void> get(const xg::Guid& id) override {
    if (auto it = db_.find(id); it != db_.end()) {
      return it->second;
    }
    return nullptr;
  }

private:
  std::unordered_map<xg::Guid, std::shared_ptr<void>> db_;
};

int main() {
  auto contentDb = std::make_shared<FakeContentDb>();
  shared::initializeContentDb(contentDb);

  auto mobDefStr = R"({
"id": {"g": "b5f8b8ee-d67b-4312-b8cf-944934342004"},
"name": "Wolf",
"rarity": 0
})";

  auto spawnDefStr = R"({
"id":{"g":"c119994e-d152-406c-9aa3-8e3b3a555151"},
"positions":[{"x":0.0,"y":0.0,"z":0.0}],
"probabilities":[
  {"f": {"g": "b5f8b8ee-d67b-4312-b8cf-944934342004"}, "s":1.0}
],
"minSpawnCount":1,
"maxSpawnCount":1,
"minSpawnAtOnce":1,
"maxSpawnAtOnce":1,
"timeBetweenSpawns":2.0})";

  {
    auto reader = shared::serialization::createJsonReader(mobDefStr);
    server::design_data::MobDef def;
    [[maybe_unused]] auto r = shared::serialization::deserialize(*reader, def);
    contentDb->registerItem(std::make_shared<server::design_data::MobDef>(def));
  }

  {
    auto reader = shared::serialization::createJsonReader(spawnDefStr);
    server::design_data::SpawnDef def;
    [[maybe_unused]] auto r = shared::serialization::deserialize(*reader, def);
    contentDb->registerItem(std::make_shared<server::design_data::SpawnDef>(def));
  }

  server::design_data::SpawnDefPtr p{xg::Guid("c119994e-d152-406c-9aa3-8e3b3a555151")};

  auto writer = shared::serialization::createJsonWriter();
  auto _ = shared::serialization::serialize(*writer, *p);
  std::cout << writer->data() << std::endl;
  std::cout << p->timeBetweenSpawns << std::endl;
  return 0;
}