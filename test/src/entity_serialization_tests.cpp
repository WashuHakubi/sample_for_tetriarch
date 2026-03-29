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
  static constexpr std::string_view typeName = "TestComponent";

  int startCount{0};
  int updateCount{0};
  int postUpdateCount{0};

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_tuple("startCount", &TestComponent::startCount),
        std::make_tuple("updateCount", &TestComponent::updateCount),
        std::make_tuple("postUpdateCount", &TestComponent::postUpdateCount));
  }
};
} // namespace

TEST_CASE("Can serialize empty entity") {
  auto go = Entity::create("go");

  auto writer = createJsonWriter();
  write(*writer, go);

  auto reader = createJsonReader(writer->toBuffer());

  auto gor = Entity::createEmpty();
  read(*reader, *gor);

  REQUIRE(gor->name() == go->name());
  REQUIRE(gor->children().size() == go->children().size());
  REQUIRE(gor->enabledSelf());

  auto writer2 = createJsonWriter();
  write(*writer2, gor);

  REQUIRE(writer2->toBuffer() == writer->toBuffer());
}

TEST_CASE("Can serialize entity with component") {
  ComponentFactories::registerFactory(TestComponent::typeName, []() { return std::make_shared<TestComponent>(); });

  auto go = Entity::create("go");
  auto goChild = Entity::create("child");
  goChild->setParent(go);
  auto comp = go->addComponent(std::make_shared<TestComponent>());
  comp->startCount = 1;
  comp->updateCount = 2;
  comp->postUpdateCount = 3;

  auto writer = createJsonWriter();
  write(*writer, go);

  auto reader = createJsonReader(writer->toBuffer());
  // std::cout << writer->toBuffer() << std::endl;

  auto gor = Entity::createEmpty();
  read(*reader, *gor);

  REQUIRE(gor->children().size() == go->children().size());
  REQUIRE(gor->children()[0]->name() == go->children()[0]->name());
  auto compr = gor->component<TestComponent>();
  REQUIRE(compr != nullptr);
  REQUIRE(compr->startCount == 1);
  REQUIRE(compr->updateCount == 2);
  REQUIRE(compr->postUpdateCount == 3);
  REQUIRE(compr->enabled());

  auto writer2 = createJsonWriter();
  write(*writer2, gor);

  REQUIRE(writer2->toBuffer() == writer->toBuffer());
}
