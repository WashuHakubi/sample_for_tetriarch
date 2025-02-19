/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "game/component/spell.h"

EWOK_REGISTRATION {
  using namespace ewok;

  Register::enum_<DamageType>("DamageType")
      .value(DamageType::None, "None")
      .value(DamageType::Air, "Air")
      .value(DamageType::Earth, "Earth")
      .value(DamageType::Fire, "Fire")
      .value(DamageType::Water, "Water");

  Register::enum_<ActionType>("ActionType")
      .value(ActionType::Aoe, "Aoe")
      .value(ActionType::Projectile, "Projectile")
      .value(ActionType::PiercingProjectile, "PiercingProjectile")
      .value(ActionType::Target, "Target");

  Register::enum_<SpellOnHitEffect>("SpellOnHitEffect")
      .value(SpellOnHitEffect::Damage, "Damage")
      .value(SpellOnHitEffect::Dot, "Dot")
      .value(SpellOnHitEffect::Heal, "Heal");

  Register::class_<SpellActionOnHit>("SpellActionOnHit")
      .field(&SpellActionOnHit::type, "type")
      .field(&SpellActionOnHit::effect, "effect")
      .field(&SpellActionOnHit::damageRange, "damageRange")
      .field(&SpellActionOnHit::duration, "duration");

  Register::class_<SpellAction>("SpellAction")
      .field(&SpellAction::type, "type")
      .field(&SpellAction::onHit, "onHit");

  Register::class_<Spell>("spell")
      .field(&Spell::castTime_, "castTime")
      .field(&Spell::actions_, "actions");
}

namespace ewok {} // namespace ewok
