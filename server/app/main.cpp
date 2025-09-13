/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <numeric>

#include <shared/hash.h>
#include <shared/content_db.h>
#include <shared/math.h>
#include <shared/message_dispatch.h>

#include <server/design_data/design_data.h>
#include <server/systems/spawn_system.h>
#include <server/systems/mob_system.h>


#include <ng-log/logging.h>

#include "yojimbo.h"

using namespace ewok;

struct FakeContentDb : shared::IContentDb {
  template <class T>
    requires(std::is_base_of_v<shared::ContentDef, T>)
  void registerItem(T const* p, shared::ContentScope scope) {
    auto id = p->id;
    db_.emplace(std::make_pair(id, std::type_index{typeid(T)}), p);

    const auto [items, _] = scopedContent_.emplace(
        std::make_pair(std::type_index{typeid(T)}, scope),
        std::vector<void const*>{});
    items->second.push_back(p);
  }

protected:
  auto get(xg::Guid const& id, std::type_index type) -> void const* override {
    if (auto const it = db_.find({id, type}); it != db_.end()) {
      return it->second;
    }

    LOG(FATAL) << "Could not find content item matching: " << id << " and " << type.name();
  }

  std::vector<void const*> getAllInScope(std::type_index type, shared::ContentScope scope) override {
    if (auto const it = scopedContent_.find({type, scope}); it != scopedContent_.end()) {
      return it->second;
    }

    return {};
  }

private:
  std::unordered_map<std::pair<xg::Guid, std::type_index>, void const*> db_;
  std::unordered_map<std::pair<std::type_index, shared::ContentScope>, std::vector<void const*>> scopedContent_;
};

int main(int argc, char** argv) {
  nglog::InitializeLogging(argv[0]);
  nglog::LogToStderr();

  auto contentDb = std::make_shared<FakeContentDb>();

  std::string_view mobDefStr = R"({
  "id": {
    "g": "b5f8b8ee-d67b-4312-b8cf-944934342004"
  },
  "name": "Wolf",
  "rarity": 0,
  "health": 100,
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
  "maxSpawnCount": 5,
  "minSpawnAtOnce": 1,
  "maxSpawnAtOnce": 1,
  "timeBetweenSpawns": 2.0
})";

  auto mobDef = std::make_shared<shared::design_data::MobDef>();
  auto spawnDef = std::make_shared<server::design_data::SpawnDef>();
  auto reader = shared::serialization::createJsonReader(mobDefStr);

  {
    [[maybe_unused]] auto r = shared::serialization::deserialize(*reader, *mobDef);
    assert(r.has_value());
    contentDb->registerItem(mobDef.get(), shared::ContentScope::Global);
  }

  {
    reader->reset(spawnDefStr);

    [[maybe_unused]] auto r = shared::serialization::deserialize(*reader, *spawnDef);
    assert(r.has_value());
    contentDb->registerItem(spawnDef.get(), shared::ContentScope::Map);
  }

  server::design_data::SpawnDefPtr p{xg::Guid("c119994e-d152-406c-9aa3-8e3b3a555151")};

  {
    auto writer = shared::serialization::createJsonWriter(true);
    [[maybe_unused]] auto r = shared::serialization::serialize(*writer, *p.resolve(*contentDb));
    assert(r.has_value());
    std::cout << writer->data() << std::endl;
  }

  // MobSystem must exist before SpawnSystem, or else the initialization needs to be handled separately from construction.
  auto mobSystem = std::make_shared<server::MobSystem>(contentDb);
  auto spawnSystem = std::make_shared<server::SpawnSystem>(contentDb);

  // All mobs should have spawned.
  auto const& mobs = server::MobSystemDebug::debugGetSpawns(*mobSystem);
  assert(mobs.size() >= 3);

  auto countDeadMobs = [&]() {
    return std::accumulate(
        mobs.begin(),
        mobs.end(),
        0,
        [](auto lhs, auto const& mob) {
          return lhs + (mob.dead ? 1 : 0);
        });
  };

  // There should be no dead mobs yet.
  assert(countDeadMobs() == 0);

  // Kill some mobs
  for (auto i = 0u; i < 3; ++i) {
    assert(!mobs[i].dead);
    shared::sendMessage(server::MobDamageRequest{i, 1000});
  }

  // 3 mobs should be dead. We require between 4 and 5 mobs and we killed 3, so we either have 1 or 2 mobs alive.
  assert(countDeadMobs() == 3);
  assert(mobs.size() - countDeadMobs() >= 1);

  // No mobs should spawn
  spawnSystem->update(1);
  assert(countDeadMobs() == 3);

  // A mob should have spawned
  spawnSystem->update(1);
  assert(countDeadMobs() == 2);

  // No more mobs should have spawned yet
  spawnSystem->update(1);
  assert(countDeadMobs() == 2);

  // Another mob should have spawned
  spawnSystem->update(1);
  assert(countDeadMobs() == 1);

  // Another mob should have spawned if we only had 4 mobs
  spawnSystem->update(1);
  spawnSystem->update(1);
  assert((mobs.size() == 5 && countDeadMobs() == 1) || countDeadMobs() == 0);
  return 0;
}