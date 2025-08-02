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
  int a{};
};

template <>
struct serialization::CustomSerializable<B> : std::true_type {
  static auto serialize(TSerializeWriter auto& writer, B const& value) {
    return writer.write("a", value.a);
  }

  static auto deserialize(TSerializeReader auto& reader, B& value) {
    return reader.read("a", value.a);
  }
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
  int a{};
  float f{};
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

struct TestAdditionalFieldTypesWriter final : serialization::Writer<TestAdditionalFieldTypesWriter> {
  TestAdditionalFieldTypesWriter()
    : writer_(serialization::createJsonWriter()) {
  }

  auto enter(std::string_view name) -> serialization::Result override {
    return writer_->enter(name);
  }

  auto leave(std::string_view name) -> serialization::Result override {
    return writer_->leave(name);
  }

  template <class T>
  auto write(std::string_view name, T value) -> serialization::Result {
    return writer_->write(name, value);
  }

  auto write(std::string_view name, std::shared_ptr<int> const& p) -> serialization::Result {
    if (!p)
      return {};
    return writer_->write(name, *p);
  }

  auto write(std::string_view name, std::unique_ptr<int> const& p) -> serialization::Result {
    if (!p)
      return {};
    return writer_->write(name, *p);
  }

  auto data() -> std::string override {
    return writer_->data();
  }

private:
  std::shared_ptr<serialization::IWriter> writer_;
};

struct TestAdditionalFieldTypesReader : serialization::Reader<TestAdditionalFieldTypesReader> {
  explicit TestAdditionalFieldTypesReader(std::string const& data)
    : reader_(serialization::createJsonReader(data)) {
  }

  auto enter(std::string_view name) -> serialization::Result override {
    return reader_->enter(name);
  }

  auto leave(std::string_view name) -> serialization::Result override {
    return reader_->leave(name);
  }

  template <class T>
  auto read(std::string_view name, T& value) -> serialization::Result {
    return reader_->read(name, value);
  }

  auto read(std::string_view name, std::shared_ptr<int>& p) -> serialization::Result {
    int v{0};
    if (auto r = reader_->read(name, v); !r) {
      if (r.error() != serialization::Error::FieldNotFound) {
        return r;
      }

      // If the field does not exist then the pointer was null.
      return {};
    }

    p = std::make_shared<int>(v);
    return {};
  }

  auto read(std::string_view name, std::unique_ptr<int>& p) -> serialization::Result {
    int v;
    if (auto r = reader_->read(name, v); !r) {
      if (r.error() != serialization::Error::FieldNotFound) {
        return r;
      }

      // If the field does not exist then the pointer was null.
      return {};
    }

    p = std::make_unique<int>(v);
    return {};
  }

private:
  std::shared_ptr<serialization::IReader> reader_;
};

struct C {
  std::unique_ptr<int> p;
  std::shared_ptr<int> s;

  static auto serializeMembers() {
    return std::make_tuple(std::make_pair("p", &C::p), std::make_pair("s", &C::s));
  }
};

TEST_CASE("Can have user defined serialization of fields") {
  C c{std::make_unique<int>(1)};
  auto writer = TestAdditionalFieldTypesWriter{};
  auto r = serialization::serialize(writer, c);
  REQUIRE(r.has_value());

  auto data = writer.data();
  REQUIRE(data == R"({"p":1})");

  C c2;
  auto reader = TestAdditionalFieldTypesReader{data};
  r = serialization::deserialize(reader, c2);
  REQUIRE(r.has_value());
  REQUIRE(*c2.p == 1);
  REQUIRE(c2.s == nullptr);
}
