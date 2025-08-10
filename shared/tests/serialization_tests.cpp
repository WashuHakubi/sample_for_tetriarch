/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <compare>
#include <tuple>

#include <catch2/catch_all.hpp>
#include <shared/serialization.h>

using namespace ewok::shared;

struct B {
  int a{};

  auto operator<=>(const B&) const = default;
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

  auto operator<=>(const Vec2&) const = default;
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

  auto operator<=>(const A&) const = default;
};

TEST_CASE("Can serialize/deserialize json") {
  A a{1, 2, "3", {42}, {6, 7}};
  auto writer = serialization::createJsonWriter();
  auto r = serialization::serialize(*writer, a);
  REQUIRE(r.has_value());

  auto data = writer->data();
  REQUIRE(data == R"({"a":1,"f":2.0,"s":"3","b":{"a":42},"v":{"x":6.0,"y":7.0}})");

  A a2;
  auto reader = serialization::createJsonReader(data);
  r = serialization::deserialize(*reader, a2);
  REQUIRE(r.has_value());
  REQUIRE(a2.a == 1);
  REQUIRE(a2.f == 2.0);
  REQUIRE(a2.s == "3");
  REQUIRE(a2.b.a == 42);
  REQUIRE(a2.v.x == 6.0);
  REQUIRE(a2.v.y == 7.0);
}

TEST_CASE("Can serialize/deserialize binary") {
  A a{1, 2, "3", {42}, {6, 7}};
  std::string buffer;
  auto writer = serialization::createBinWriter(buffer, true);
  auto r = serialization::serialize(*writer, a);
  REQUIRE(r.has_value());

  auto const expectedSize =
      sizeof(a.a) + // 4
      sizeof(a.f) + // 4
      sizeof(uint32_t) + a.s.size() + // strlen + str, 4 + 1
      sizeof(B) + // 4
      sizeof(Vec2); // 8

  auto data = writer->data();
  REQUIRE(data == buffer);
  REQUIRE(data.size() == expectedSize);

  A a2;
  auto reader = serialization::createBinReader(data, true);
  r = serialization::deserialize(*reader, a2);
  REQUIRE(r.has_value());
  REQUIRE(a2.a == 1);
  REQUIRE(a2.f == 2.0);
  REQUIRE(a2.s == "3");
  REQUIRE(a2.b.a == 42);
  REQUIRE(a2.v.x == 6.0);
  REQUIRE(a2.v.y == 7.0);

  auto aFieldMapping = writer->fieldMapping();
  auto bFieldMapping = reader->fieldMapping();
  REQUIRE(aFieldMapping == bFieldMapping);
}

// This writer depends on being able to detect missing fields.
struct TestAdditionalFieldTypesWriter final : serialization::Writer<TestAdditionalFieldTypesWriter> {
  TestAdditionalFieldTypesWriter()
    : writer_(serialization::createJsonWriter()) {
  }

  auto array(std::string_view name, size_t count) -> serialization::Result override {
    return writer_->array(name, count);
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

  void reset() override {
    writer_->reset();
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

  auto array(std::string_view name, size_t& count) -> serialization::Result override {
    return reader_->array(name, count);
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

TEST_CASE("Can json serialize primitive arrays") {
  struct S {
    std::vector<int> a;

    static auto serializeMembers() {
      return std::make_tuple(
          std::make_pair("a", &S::a));
    }
  };

  S s{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}};
  const auto writer = serialization::createJsonWriter();
  auto r = serialization::serialize(*writer, s);
  REQUIRE(r.has_value());

  auto data = writer->data();
  REQUIRE(data == R"({"a":[1,2,3,4,5,6,7,8,9,10]})");

  S s2{};
  const auto reader = serialization::createJsonReader(data);
  r = serialization::deserialize(*reader, s2);
  REQUIRE(r.has_value());
  REQUIRE(s2.a == std::vector{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
}

TEST_CASE("Can json serialize complex arrays") {
  struct S {
    std::vector<A> a;

    static auto serializeMembers() {
      return std::make_tuple(
          std::make_pair("a", &S::a));
    }
  };

  S s{{
      {1, 2, "3", {42}, {6, 7}},
      {2, 3, "4", {84}, {7, 8}}
  }};

  const auto writer = serialization::createJsonWriter();
  auto r = serialization::serialize(*writer, s);
  REQUIRE(r.has_value());

  auto data = writer->data();
  REQUIRE(
      data ==
      R"({"a":[{"a":1,"f":2.0,"s":"3","b":{"a":42},"v":{"x":6.0,"y":7.0}},{"a":2,"f":3.0,"s":"4","b":{"a":84},"v":{"x":7.0,"y":8.0}}]})")
  ;

  S s2{};
  const auto reader = serialization::createJsonReader(data);
  r = serialization::deserialize(*reader, s2);
  REQUIRE(r.has_value());
  REQUIRE(s2.a == s.a);
}

TEST_CASE("Can binary serialize primitive arrays") {
  struct S {
    std::vector<int> a;

    static auto serializeMembers() {
      return std::make_tuple(
          std::make_pair("a", &S::a));
    }
  };

  S s{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}};
  std::string buffer;
  const auto writer = serialization::createBinWriter(buffer);
  auto r = serialization::serialize(*writer, s);
  REQUIRE(r.has_value());

  auto const expectedSize =
      sizeof(uint32_t) // count prefix for the array
      + s.a.size() * sizeof(int); // number of items in the array * size of each item.

  REQUIRE(buffer.size() == expectedSize);

  S s2{};
  const auto reader = serialization::createBinReader(buffer);
  r = serialization::deserialize(*reader, s2);
  REQUIRE(r.has_value());
  REQUIRE(s2.a == std::vector{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
}

TEST_CASE("Can binary serialize complex arrays") {
  struct S {
    std::vector<A> a;

    static auto serializeMembers() {
      return std::make_tuple(
          std::make_pair("a", &S::a));
    }
  };

  S s{{
      {1, 2, "3", {42}, {6, 7}},
      {2, 3, "4", {84}, {7, 8}}
  }};

  std::string buffer;
  const auto writer = serialization::createBinWriter(buffer);
  auto r = serialization::serialize(*writer, s);
  REQUIRE(r.has_value());

  // We expect the size to be the array count + the size of each individual item in the array.
  auto const expectedSize =
      sizeof(uint32_t) + // count of array items
      sizeof(A::a) * 2 + // 4
      sizeof(A::f) * 2 + // 4
      sizeof(uint32_t) * 2 + s.a[0].s.size() + s.a[1].s.size() + // strlen + str, 4 + 1
      sizeof(B) * 2 + // 4
      sizeof(Vec2) * 2; // 8

  REQUIRE(buffer.size() == expectedSize);

  S s2{};
  const auto reader = serialization::createBinReader(buffer);
  r = serialization::deserialize(*reader, s2);
  REQUIRE(r.has_value());
  REQUIRE(s2.a == s.a);
}