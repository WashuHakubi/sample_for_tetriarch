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

struct Heal {
  float min;
  float max;
};

struct DirectDamage {
  float min;
  float max;
};

struct DotDamage {
  float duration;
  float amount;
};

using SpellDamage = std::variant<DirectDamage, DotDamage, Heal>;
struct SpellActionOnHit {
  SpellDamage damage;
};

struct SpellAction {
  ActionType type;
  std::vector<SpellActionOnHit> onHit;
};

template <class Cont, class Pred>
void eraseIf(Cont& container, Pred pred) {
  auto first = std::begin(container);
  auto last = std::end(container);

  for (; first != last; ++first) {
    if (pred(*first)) {
      break;
    }
  }

  if (first == last) {
    return;
  }

  for (auto it = first; ++it != last;) {
    if (!pred(*it)) {
      *first++ = std::move(*it);
    }
  }

  container.erase(first, last);
}

struct Health {
  float current;
  float max;

  std::vector<DotDamage> tickEffects;

  void update(float dt) {
    if (tickEffects.empty()) {
      return;
    }

    // Iterate over all the ticking effects, apply them, and then erase them
    eraseIf(tickEffects, [this, dt](auto&& dd) {
      current = std::max(0.f, current - dd.amount) * dt;
      dd.duration -= dt;
      // True if we should remove this (i.e. the duration has hit 0)
      return dd.duration <= 0;
    });
  }
};

void apply(Health& h, SpellDamage const& damage) {
  std::visit(
      [&h](auto&& spellDamage) {
        using T = std::decay_t<decltype(spellDamage)>;
        if constexpr (std::is_same_v<T, DirectDamage>) {
          // Imagine getting the damage between min/max
          auto dmg = spellDamage.min + (spellDamage.max - spellDamage.min) / 2;
          h.current = std::max(h.current - dmg, 0.f);
        } else if constexpr (std::is_same_v<T, DotDamage>) {
          h.tickEffects.push_back(spellDamage);
        } else if constexpr (std::is_same_v<T, Heal>) {
          auto heal = spellDamage.min + (spellDamage.max - spellDamage.min) / 2;
          h.current = std::min(h.current + heal, h.max);
        } else {
          static_assert(false, "This damage type must be implemented.");
        }
      },
      damage);
}

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
