/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <random>

#include "design_data/design_data.h"
#include "shared/content_db.h"
#include "shared/math.h"
#include "shared/message_dispatch.h"

using namespace ewok;

namespace ewok::server {
class MobSystem;
class SpawnSystem;

struct MobKilled {
  uint32_t id;
  uint32_t spawnId;
};

struct SpawnMobRequest {
  shared::design_data::MobDefPtr mob;
  uint32_t spawnId;
  glm::vec3 position;
  glm::quat rotation;
};

struct DamageMobRequest {
  uint32_t id;
  int32_t amount;
};

struct MobHealthChanged {
  uint32_t id;
  int32_t curHealth;
};

struct MobSpawned {
  uint32_t id;
  uint32_t spawnId;
  shared::design_data::MobDefPtr mob;
  int32_t curHealth;
  glm::vec3 position;
  glm::quat rotation;
};

struct Random {
  static uint32_t next(uint32_t min, uint32_t max) { return std::uniform_int_distribution{min, max}(s_gen); }

  static float nextReal(float min = 0, float max = 1) { return std::uniform_real_distribution{min, max}(s_gen); }

 private:
  static std::mt19937 s_gen;
};

struct SpawnData {
  SpawnData(design_data::SpawnDefPtr spawnDef)
      : spawnDef(std::move(spawnDef)), minSpawnCount(this->spawnDef->minSpawnCount) {}

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
  SpawnSystem() {
    msgHandle_ = shared::subscribeMessage([this](MobKilled const& msg) { this->onMobKilled(msg); });
  }

  void update(float dt) {
    for (uint32_t id = 0; id < spawns_.size(); ++id) {
      auto& spawn = spawns_[id];

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
          auto spawnCount = Random::next(spawn.spawnDef->minSpawnAtOnce, spawn.spawnDef->maxSpawnAtOnce);

          // Make sure we don't exceed the max spawn count
          const auto nextSpawnCount = std::max(spawn.curSpawnCount + spawnCount, spawn.spawnDef->maxSpawnCount);

          // and recompute the number of mobs we need to spawn
          spawnCount = nextSpawnCount - spawn.curSpawnCount;

          // Get the sum of the total probabilities for the mob spawns.
          const auto probSum = std::accumulate(
              spawn.spawnDef->spawnProbabilities.begin(),
              spawn.spawnDef->spawnProbabilities.end(),
              0.0f,
              [](auto lhs, auto const& rhs) { return lhs + rhs.second; });

          for (auto i = 0u; i < spawnCount; ++i) {
            auto spawnPosIdx = Random::next(0, spawn.spawnDef->spawnPositions.size());
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
                shared::sendMessage(SpawnMobRequest{mob, id, spawnPos, glm::quat{}});
                break;
              }
            }
          }

          // Update our spawn count to the number of mobs spawned.
          spawn.curSpawnCount = nextSpawnCount;

          // This ensures we don't do large spawns repeatedly.
          spawn.needsSpawn = false;
        }
      }
    }
  }

 private:
  void onMobKilled(MobKilled const& mobKilled) {
    auto& spawn = spawns_[mobKilled.spawnId];
    --spawn.curSpawnCount;
    assert(spawn.curSpawnCount >= 0);
  }

  std::vector<SpawnData> spawns_{};
  shared::MsgDispatchHandle<MobKilled> msgHandle_;
};

struct MobData {
  MobData(uint32_t spawnId, shared::design_data::MobDefPtr mobDef, glm::vec3 position, glm::quat rotation)
      : spawnId(spawnId), mobDef(std::move(mobDef)), position(position), rotation(rotation) {
    curHealth = maxHealth = static_cast<int32_t>(mobDef->health);
  }

  bool dead{false};

  uint32_t spawnId;

  shared::design_data::MobDefPtr mobDef;

  glm::vec3 position{};

  glm::quat rotation{};

  int32_t maxHealth{};

  int32_t curHealth{};
};

class MobSystem {
 public:
  MobSystem() {
    spawnMobRequest_ = shared::subscribeMessage([this](SpawnMobRequest const& msg) { onSpawnMobRequest(msg); });

    damageMobRequest_ = shared::subscribeMessage([this](DamageMobRequest const& msg) { onDamageMobRequest(msg); });
  }

 private:
  void onSpawnMobRequest(SpawnMobRequest const& req) {
    uint32_t id;
    MobData const* mob;
    if (freeIds_.empty()) {
      id = mobs_.size();
      mob = &mobs_.emplace_back(req.spawnId, req.mob, req.position, req.rotation);
    } else {
      id = freeIds_.back();
      freeIds_.pop_back();
      mobs_[id] = {req.spawnId, req.mob, req.position, req.rotation};
      mob = &mobs_[id];
    }

    shared::sendMessage(MobSpawned{id, mob->spawnId, mob->mobDef, mob->curHealth, mob->position, mob->rotation});
  }

  void onDamageMobRequest(DamageMobRequest const& req) {
    auto& mob = mobs_[req.id];

    if (mob.dead) {
      return;
    }

    mob.curHealth = std::clamp(mob.curHealth - req.amount, 0, mob.maxHealth);
    shared::sendMessage(MobHealthChanged{req.id, mob.curHealth});

    if (mob.curHealth <= 0) {
      mob.dead = true;
      freeIds_.push_back(req.id);
      shared::sendMessage(MobKilled{req.id, mob.spawnId});
    }
  }

  std::vector<MobData> mobs_;
  std::vector<uint32_t> freeIds_;
  shared::MsgDispatchHandle<SpawnMobRequest> spawnMobRequest_;
  shared::MsgDispatchHandle<DamageMobRequest> damageMobRequest_;
};

} // namespace ewok::server

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
    abort();
  }

 private:
  std::unordered_map<xg::Guid, std::shared_ptr<void>> db_;
};

int main() {
  auto contentDb = std::make_shared<FakeContentDb>();
  shared::initializeContentDb(contentDb);

  std::string_view mobDefStr = R"({
  "id": {
    "g": "b5f8b8ee-d67b-4312-b8cf-944934342004"
  },
  "name": "Wolf",
  "rarity": 0,
  "maxHealth": 100,
  "model": "wolf.prefab"
})";

  std::string_view spawnDefStr = R"({
  "id": {
    "g": "c119994e-d152-406c-9aa3-8e3b3a555151"
  },
  "positions": [
    {
      "x": 0.0,
      "y": 0.0,
      "z": 0.0
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
  "minSpawnCount": 1,
  "maxSpawnCount": 1,
  "minSpawnAtOnce": 1,
  "maxSpawnAtOnce": 1,
  "timeBetweenSpawns": 2.0
})";

  auto reader = shared::serialization::createJsonReader(mobDefStr);
  {
    auto def = std::make_shared<shared::design_data::MobDef>();
    [[maybe_unused]] auto r = shared::serialization::deserialize(*reader, *def);
    assert(r.has_value());
    contentDb->registerItem(std::move(def));
  }

  {
    reader->reset(spawnDefStr);
    auto def = std::make_shared<server::design_data::SpawnDef>();
    [[maybe_unused]] auto r = shared::serialization::deserialize(*reader, *def);
    assert(r.has_value());
    contentDb->registerItem(std::move(def));
  }

  server::design_data::SpawnDefPtr p{xg::Guid("c119994e-d152-406c-9aa3-8e3b3a555151")};

  {
    auto writer = shared::serialization::createJsonWriter(true);
    [[maybe_unused]] auto r = shared::serialization::serialize(*writer, *p);
    assert(r.has_value());
    std::cout << writer->data() << std::endl;
  }

  return 0;
}