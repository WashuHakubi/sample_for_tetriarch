/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <tuple>
#include <catch2/catch_all.hpp>
#include <shared/serialization.h>

using namespace ewok::shared;

struct B {
  int a;

  auto serialize(serialization::TSerializeWriter auto& writer) const {
    return writer.write("a", a);
  }

  auto deserialize(serialization::TSerializeReader auto& reader) {
    return reader.read("a", a);
  }
};

template <>
struct serialization::IsCustomSerializable<B> : std::true_type {
};

struct Vec2 {
  float x{};
  float y{};

  static auto serializeMembers() {
    return std::tuple{
        std::make_pair("x", &Vec2::x),
        std::make_pair("y", &Vec2::y),
    };
  }
};

struct A {
  int a;
  float f;
  std::string s;
  B b;
  Vec2 v;

  static auto serializeMembers() {
    return std::tuple{
        std::make_pair("a", &A::a),
        std::make_pair("f", &A::f),
        std::make_pair("s", &A::s),
        std::make_pair("b", &A::b),
        std::make_pair("v", &A::v),
    };
  }
};

TEST_CASE("Can serialize/deserialize json") {
  A a{1, 2, "3", {42}, {6, 7}};
  auto writer = serialization::createJsonWriter();
  auto r = serialization::serialize(*writer, a);
  REQUIRE(r.has_value());

  auto data = writer->data();
  REQUIRE(data == R"({"a":1,"b":{"a":42},"f":2.0,"s":"3","v":{"x":6.0,"y":7.0}})");

  A b;
  auto reader = serialization::createJsonReader(data);
  r = serialization::deserialize(*reader, b);
  REQUIRE(r.has_value());
  REQUIRE(b.a == 1);
  REQUIRE(b.f == 2.0);
  REQUIRE(b.s == "3");
  REQUIRE(b.b.a == 42);
  REQUIRE(b.v.x == 6.0);
  REQUIRE(b.v.y == 7.0);
}

namespace ewok::shared::serialization {
auto serialize_field(
    TSerializeWriter auto& writer,
    std::string_view name,
    std::unique_ptr<int> const& value) -> Result {
  return writer.write("p", *value);
}

auto deserialize_field(
    TSerializeReader auto& reader,
    std::string_view name,
    std::unique_ptr<int>& value) -> Result {
  int val;
  auto r = reader.read("p", val);
  if (!r)
    return r;
  value = std::make_unique<int>(val);
  return {};
}
}

struct C {
  std::unique_ptr<int> p;

  static auto serializeMembers() {
    return std::make_tuple(std::make_pair("p", &C::p));
  }
};

TEST_CASE("Can specialize serialization") {
  C c{std::make_unique<int>(1)};
  auto writer = serialization::createJsonWriter();
  auto r = serialization::serialize(*writer, c);
  REQUIRE(r.has_value());

  auto data = writer->data();
  REQUIRE(data == R"({"p":1})");

  C c2;
  auto reader = serialization::createJsonReader(data);
  r = serialization::deserialize(*reader, c2);
  REQUIRE(r.has_value());
  REQUIRE(*c2.p == 1);
}