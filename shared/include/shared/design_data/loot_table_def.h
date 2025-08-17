/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

namespace ewok::shared::design_data {
enum class LootTableItemType {
  Item,
  Table,
};

struct LootTableItem {
  // We either point to an item, or to a nested loot table.
  ContentVariant<LootTableItemType, ItemDefPtr, LootTableDefPtr> item;
  float weight;

  static constexpr auto serializationMembers() {
    return std::make_tuple(
        std::make_pair("type", &LootTableItem::item),
        std::make_pair("weight", &LootTableItem::weight));
  }
};

struct LootTableDef : ContentDef {
  LootTableDef() = default;

  LootTableDef(xg::Guid id, std::vector<LootTableItem> items)
    : ContentDef{id},
      items(std::move(items)) {
  }

  std::vector<LootTableItem> items;

  static constexpr auto serializationMembers() {
    return std::make_tuple(
        std::make_pair("id", &LootTableDef::id),
        std::make_pair("items", &LootTableDef::items));
  }

  /// Picks a random entry from the loot table.
  auto pick(auto& rng) const noexcept -> LootTableItem const& {
    std::uniform_real_distribution dist{0.0f, totalWeight()};

    auto wanted = dist(rng);
    auto weightSum = 0.0f;

    for (auto const& item : items) {
      weightSum += item.weight;
      if (wanted <= weightSum) {
        return item;
      }
    }

    return items.back();
  }

  /// Picks a random item from the loot table and any child tables.
  /// This is actually subtly wrong, as it rolls multiple times as it traverses down the tree of tables.
  /// A more correct form would roll once based on the cumulative total weight and then pick appropriately from within
  /// the set of all loot tables.
  auto pickItem(auto& rng, IContentDb& db) const noexcept -> ItemDefPtr const& {
    auto const& entry = pick(rng);

    if (entry.item.type() == LootTableItemType::Item) {
      // We have an item, return it
      return std::get<ItemDefPtr>(entry.item);
    }

    // Recurse into the child loot table and continue our picking.
    return std::get<LootTableDefPtr>(entry.item).resolve(db)->pickItem(rng, db);
  }

private:
  /// Computes the total weight of all the items in the table. Weights should never change at runtime.
  auto totalWeight() const noexcept -> float {
    if (totalWeight_ > 0) [[likely]] { return totalWeight_; }

    totalWeight_ = std::accumulate(
        items.begin(),
        items.end(),
        0.0f,
        [](auto l, auto const& r) { return l + r.weight; });
    return totalWeight_;
  }

  // Cache variable to avoid recomputing the total weight every time we want to pick an item.
  mutable float totalWeight_ = 0;
};
}