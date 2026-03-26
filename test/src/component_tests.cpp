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
  int startCount{0};
  int updateCount{0};
  int postUpdateCount{0};

  void onStart() override { ++startCount; }

  void update() override { ++updateCount; }

  void postUpdate() override { ++postUpdateCount; }

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_tuple("startCount", &TestComponent::startCount),
        std::make_tuple("updateCount", &TestComponent::updateCount),
        std::make_tuple("postUpdateCount", &TestComponent::postUpdateCount));
  }
};

struct TestAddsComponentInUpdate final : ComponentT<TestAddsComponentInUpdate> {
  bool added{false};

  void update() override {
    if (!added) {
      parent()->addComponent(std::make_shared<TestComponent>());
      added = true;
    }
  }

  static auto serializeMembers() {
    return std::make_tuple(std::make_tuple("added", &TestAddsComponentInUpdate::added));
  }
};
} // namespace

TEST_CASE("Can add components to entity") {
  auto go = Entity::create("go");

  auto c = std::make_shared<TestComponent>();
  REQUIRE(c->enabled());
  REQUIRE(c->parent() == nullptr);
  REQUIRE(c->startCount == 0);
  REQUIRE(c->updateCount == 0);
  REQUIRE(c->postUpdateCount == 0);

  go->addComponent(c);
  REQUIRE(c->parent() == go);

  REQUIRE(c->startCount == 0);
  REQUIRE(c->updateCount == 0);
  REQUIRE(c->postUpdateCount == 0);

  REQUIRE(go->component<TestComponent>() == c);

  go->update();
  REQUIRE(c->startCount == 1);
  REQUIRE(c->updateCount == 1);
  REQUIRE(c->postUpdateCount == 0);

  go->postUpdate();
  REQUIRE(c->startCount == 1);
  REQUIRE(c->updateCount == 1);
  REQUIRE(c->postUpdateCount == 1);

  go->update();
  // Start should only be called once.
  REQUIRE(c->startCount == 1);
  REQUIRE(c->updateCount == 2);
}

TEST_CASE("Component added in update is not updated") {
  auto go = Entity::create("go");
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
  auto go = Entity::create("go");
  auto c = std::make_shared<TestComponent>();

  go->addComponent(c);
  REQUIRE(c->parent() == go);

  go->update();
  go->postUpdate();
  REQUIRE(c->updateCount == 1);
  REQUIRE(c->postUpdateCount == 1);

  // Should be 3 references: ourself, go, and go by type.
  REQUIRE(c.use_count() == 3);

  REQUIRE(go->component<TestComponent>() == c);

  c->destroy();
  // Should be 2 references, ourself, and go. It should no longer be referenced by type.
  REQUIRE(c.use_count() == 2);

  // Should not be able to lookup the component by type.
  REQUIRE(go->component<TestComponent>() == nullptr);

  go->update();
  // Should not be updated
  REQUIRE(c->updateCount == 1);

  go->postUpdate();
  // This should simply remove the component and not update it.
  REQUIRE(c->postUpdateCount == 1);
  REQUIRE(c.use_count() == 1);
}
