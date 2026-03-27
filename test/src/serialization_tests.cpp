/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <catch2/catch_all.hpp>

#include <iostream>
#include <wut/serialization.h>

using namespace wut;

namespace {
struct S {
  int i;
  float f;
  std::string name;
  std::shared_ptr<S> other;
  std::shared_ptr<S> other2;
  std::vector<int> is;
  std::vector<std::shared_ptr<S>> others;

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_tuple("i", &S::i),
        std::make_tuple("f", &S::f),
        std::make_tuple("name", &S::name),
        std::make_tuple("other", &S::other),
        std::make_tuple("other2", &S::other2),
        std::make_tuple("is", &S::is),
        std::make_tuple("others", &S::others));
  }
};
} // namespace

TEST_CASE("Json serialization") {
  S s{
      .i = 42,
      .f = 3.14f,
      .name = "stuff",
      .other = std::make_shared<S>(10, 1.24f, "other", nullptr),
      .is = {1, 2, 3, 4},
  };
  s.other2 = s.other;
  s.others.push_back(s.other);
  s.others.push_back(std::make_shared<S>(1, 2.0f, "asdf"));

  auto writer = createJsonWriter();
  write(*writer, s);

  auto buf = writer->toBuffer();
  // std::cout << buf << std::endl;

  auto reader = createJsonReader(buf);

  S s2;
  read(*reader, s2);

  REQUIRE(s2.i == s.i);
  REQUIRE(s2.f == s.f);
  REQUIRE(s2.name == "stuff");
  REQUIRE(s2.is == std::vector<int>{1, 2, 3, 4});
  REQUIRE(s2.other != nullptr);
  REQUIRE(s2.other->i == s.other->i);
  REQUIRE(s2.other->f == s.other->f);
  REQUIRE(s2.other->name == "other");
  REQUIRE(s2.other->other == nullptr);
  REQUIRE(s2.other->other2 == nullptr);
  REQUIRE(s2.other2 == s2.other);
  REQUIRE(s2.others.size() == 2);
  REQUIRE(s2.others[0] == s2.other);
  REQUIRE(s2.others[1]->i == 1);
  REQUIRE(s2.others[1]->f == 2.0f);
  REQUIRE(s2.others[1]->name == "asdf");
}
