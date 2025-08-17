/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <unordered_set>

#include <catch2/catch_all.hpp>

#include <shared/design_data/design_data.h>
#include <shared/design_data/testing/fake_content_db.h>

using namespace ewok::shared::design_data;

LootTableDefPtr createLootTables(FakeContentDb& db) {
  auto item1Id = xg::newGuid();
  auto item2Id = xg::newGuid();
  auto item3Id = xg::newGuid();

  auto item1 = db.registerItem(
      std::make_shared<ItemDef>(ItemDef{{item1Id}, "item1"}),
      ewok::shared::ContentScope::Global);

  auto item2 = db.registerItem(
      std::make_shared<ItemDef>(ItemDef{{item2Id}, "item2"}),
      ewok::shared::ContentScope::Global);

  auto item3 = db.registerItem(
      std::make_shared<ItemDef>(ItemDef{{item3Id}, "item3"}),
      ewok::shared::ContentScope::Global);

  auto lootTableChildId = xg::newGuid();
  auto childTable = db.registerItem(
      std::make_shared<LootTableDef>(
          LootTableDef{
              lootTableChildId,
              {
                  // Two items each with an equal chance.
                  {{item1}, 1.0f},
                  {{item2}, 1.0f},
              }
          }),
      ewok::shared::ContentScope::Global);

  auto parentTableChildId = xg::newGuid();
  auto parentTable = db.registerItem(
      std::make_shared<LootTableDef>(
          LootTableDef{
              parentTableChildId,
              {
                  // Two items each with an equal chance.
                  {{childTable}, 1.0f},
                  {{item3}, 1.0f},
              }
          }),
      ewok::shared::ContentScope::Global);

  return parentTable;
}

template <float Min, float Max>
struct FakeRandomGen {
  using result_type = float;

  static result_type min() { return Min; }
  static result_type max() { return Max; }

  FakeRandomGen(result_type result)
    : result_(std::move(result)) {
  }

  result_type operator()() const {
    return result_;
  }

private:
  mutable size_t index_{};
  result_type result_;
};

TEST_CASE("Can pick from table") {
  auto db = std::make_shared<FakeContentDb>();
  auto parentTable = createLootTables(*db);

  auto pickHalf = FakeRandomGen<0.f, 2.f>{0.5f};
  auto pickOne = FakeRandomGen<0.f, 2.f>{1.0f};
  auto pickOneAndHalf = FakeRandomGen<0.f, 2.f>{1.5f};
  auto pickTwo = FakeRandomGen<0.f, 2.f>{2.0f};

  auto table = parentTable.resolve(*db);

  // Should pick the child table.
  {
    auto item = table->pick(pickHalf);
    REQUIRE(item.item.type() == LootTableItemType::Table);
  }

  // Should pick the child table
  {
    auto item = table->pick(pickOne);
    REQUIRE(item.item.type() == LootTableItemType::Table);
  }

  // Should pick the item.
  {
    auto item = table->pick(pickOneAndHalf);
    REQUIRE(item.item.type() == LootTableItemType::Item);
    auto itemPtr = std::get<ItemDefPtr>(item.item);
    REQUIRE(itemPtr.resolve(*db)->name == "item3");
  }

  // Should pick the item.
  {
    auto item = table->pick(pickTwo);
    REQUIRE(item.item.type() == LootTableItemType::Item);
    auto itemPtr = std::get<ItemDefPtr>(item.item);
    REQUIRE(itemPtr.resolve(*db)->name == "item3");
  }
}

TEST_CASE("Can pick item from table and child tables") {
  auto db = std::make_shared<FakeContentDb>();
  auto parentTable = createLootTables(*db);

  std::random_device rd;
  std::mt19937 gen(rd());

  std::unordered_set<std::string> itemNames;
  for (size_t i = 0; i < 1000; ++i) {
    auto item = parentTable.resolve(*db)->pickItem(gen, *db);
    itemNames.insert(item.resolve(*db)->name);
  }

  // We expect to have hit all three items after some period of choosing.
  REQUIRE(itemNames.size() == 3);
}