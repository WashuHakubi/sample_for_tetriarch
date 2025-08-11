/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "../../include/server/systems/mob_system.h"

ewok::server::MobSystem::MobSystem(shared::IContentDbPtr contentDb)
  : contentDb_(std::move(contentDb)) {
  spawnMobRequest_ = shared::subscribeMessage([this](SpawnMobRequest const& msg) { onSpawnMobRequest(msg); });
  damageMobRequest_ = shared::subscribeMessage([this](MobDamageRequest const& msg) { onDamageMobRequest(msg); });
}

void ewok::server::MobSystem::onSpawnMobRequest(SpawnMobRequest const& req) {
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

  LOG(INFO) << "Spawned mob:" << id << " " << mob->mobDef->id << " for spawn:" << mob->spawnId;
  shared::sendMessage(MobSpawned{id, mob->spawnId, mob->mobDef, mob->curHealth, mob->position, mob->rotation});
}

void ewok::server::MobSystem::onDamageMobRequest(MobDamageRequest const& req) {
  auto& mob = mobs_[req.id];

  if (mob.dead) {
    return;
  }

  mob.curHealth = std::clamp(mob.curHealth - req.amount, 0, mob.maxHealth);
  shared::sendMessage(MobHealthChanged{req.id, mob.curHealth});

  if (mob.curHealth <= 0) {
    LOG(INFO) << "mob:" << req.id << " died.";
    mob.dead = true;
    freeIds_.push_back(req.id);
    shared::sendMessage(MobKilled{req.id, mob.spawnId});
  }
}