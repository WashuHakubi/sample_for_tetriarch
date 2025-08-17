/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <numeric>
#include <random>

#include <shared/content_variant.h>
#include <shared/content_db.h>
#include <shared/math.h>

namespace ewok::shared::design_data {
struct ItemDef;
using ItemDefPtr = ContentPtr<ItemDef>;

struct LootTableDef;
using LootTableDefPtr = ContentPtr<LootTableDef>;

struct MobDef;
using MobDefPtr = ContentPtr<MobDef>;
}

#include <shared/design_data/item_def.h>
#include <shared/design_data/loot_table_def.h>
#include <shared/design_data/mob_def.h>
#include <shared/design_data/player_spawn_def.h>
