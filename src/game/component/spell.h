/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "engine/component.h"
#include "engine/reflection/reflection.h"

namespace ewok {
enum class DamageType {
  None,
  Air,
  Earth,
  Fire,
  Water,
};

enum class ActionType {
  Aoe,
  Projectile,
  PiercingProjectile,
  Target,
};

enum class SpellOnHitEffect {
  Damage,
  Dot,
  Heal,
};

struct SpellActionOnHit {
  SpellOnHitEffect effect;
  DamageType type;
  std::pair<float, float> damageRange;
  float duration;
};

struct SpellAction {
  ActionType type;
  std::vector<SpellActionOnHit> onHit;
};

class Spell : public Component<Spell> {
 public:
 private:
  EWOK_REFLECTION_DECL

  std::string name_;
  // How long the spell takes to cast
  float castTime_;

  // How long until the cast cannot be interrupted
  float interruptType_;

  // How long until the spell can be cast again
  float cooldown_;

  // How long, since the start of the spell, until the actions start to play
  // out.
  float actionStartTime_;

  int manaCost_;

  std::vector<SpellAction> actions_;
};
} // namespace ewok
