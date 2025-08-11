/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

namespace ewok::shared::design_data {
struct MobDef;
using MobDefPtr = ContentPtr<MobDef>;

enum class MobRarity {
  Normal,
  Magic,
  Rare,
  Unique
};

struct MobDef : ContentDef {
  // Name of the mob
  std::string name;

  // Rarity of the mob
  MobRarity rarity{};

  // Base health of the mob
  uint32_t health{};

  // Model prefab for this mob
  std::string model;

  static constexpr auto serializationMembers() {
    return std::make_tuple(
        std::make_pair("id", &MobDef::id),
        std::make_pair("name", &MobDef::name),
        std::make_pair("rarity", &MobDef::rarity),
        std::make_pair("health", &MobDef::health),
        std::make_pair("model", &MobDef::model));
  }
};
}