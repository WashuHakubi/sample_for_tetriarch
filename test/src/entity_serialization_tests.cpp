/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <catch2/catch_all.hpp>

#include <wut/component.h>
#include <wut/entity.h>

#include <iostream>

using namespace wut;

namespace {
struct TestComponent final : ComponentT<TestComponent> {
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
  writeObject(*writer, "", go);

  std::cout << writer->toBuffer() << std::endl;
}

TEST_CASE("Can serialize entity with component") {
  auto go = Entity::create("go");
  go->addComponent(std::make_shared<TestComponent>());

  auto writer = createJsonWriter();
  writeObject(*writer, "", go);

  std::cout << writer->toBuffer() << std::endl;
}
