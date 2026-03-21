/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <catch2/catch_all.hpp>

#include <wut/component.h>
#include <wut/entity.h>

using namespace wut;

namespace {
struct TestComponent final : ComponentT<TestComponent> {
  int updateCount{0};
  int postUpdateCount{0};

  void update() override { ++updateCount; }

  void postUpdate() override { ++postUpdateCount; }
};

struct TestAddsComponentInUpdate final : ComponentT<TestAddsComponentInUpdate> {
  bool added{false};

  void update() override {
    if (!added) {
      parent()->addComponent(std::make_shared<TestComponent>());
      added = true;
    }
  }
};
} // namespace

TEST_CASE("Can add components to entity") {
  auto go = Entity::create();

  auto c = std::make_shared<TestComponent>();
  REQUIRE(c->enabled());
  REQUIRE(c->parent() == nullptr);
  REQUIRE(c->updateCount == 0);
  REQUIRE(c->postUpdateCount == 0);

  go->addComponent(c);
  REQUIRE(c->parent() == go);

  REQUIRE(c->updateCount == 0);
  REQUIRE(c->postUpdateCount == 0);

  REQUIRE(go->component<TestComponent>() == c);

  go->update();
  REQUIRE(c->updateCount == 1);
  REQUIRE(c->postUpdateCount == 0);

  go->postUpdate();
  REQUIRE(c->updateCount == 1);
  REQUIRE(c->postUpdateCount == 1);
}

TEST_CASE("Component added in update is not updated") {
  auto go = Entity::create();
  auto c = std::make_shared<TestAddsComponentInUpdate>();

  go->addComponent(c);
  REQUIRE(c->parent() == go);

  // TestComponent should not have been added.
  REQUIRE(c->added == false);

  // Double check.
  REQUIRE(go->component<TestComponent>() == nullptr);

  // Run update.
  go->update();

  // TestComponent should have been added
  REQUIRE(c->added == true);

  // We should be able to get the component.
  auto c2 = go->component<TestComponent>();
  REQUIRE(c2 != nullptr);

  // TestComponent should not have been updated as we have not finished the "current frame" for this entity.
  REQUIRE(c2->updateCount == 0);
  REQUIRE(c2->postUpdateCount == 0);

  // TestComponent should not have been updated as we have not finished the "current frame" for this entity.
  go->postUpdate();
  REQUIRE(c2->updateCount == 0);
  REQUIRE(c2->postUpdateCount == 0);

  // The next "frame" should update the TestComponent.
  go->update();
  REQUIRE(c2->updateCount == 1);

  go->postUpdate();
  REQUIRE(c2->postUpdateCount == 1);
}

TEST_CASE("Component destroy") {
  auto go = Entity::create();
  auto c = std::make_shared<TestComponent>();

  go->addComponent(c);
  REQUIRE(c->parent() == go);

  go->update();
  go->postUpdate();
  REQUIRE(c->updateCount == 1);
  REQUIRE(c->postUpdateCount == 1);

  // Should be 3 references: ourself, go, and go by type.
  REQUIRE(c.use_count() == 3);
}
