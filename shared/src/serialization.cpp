/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shared/serialization.h"

#include <iostream>
#include <sstream>
#include <string>
#include <stack>

#include "nlohmann/json.hpp"

namespace ewok::shared {
struct JsonWriter : ISerializeWriter {
  JsonWriter()
    : root_({}) {
    json_.push(&root_);
  }

  auto enter(std::string_view name) -> SerializeResult override {
    auto& top = *json_.top();
    auto& next = top[name];
    json_.push(&next);
    return {};
  }

  auto leave(std::string_view name) -> SerializeResult override {
    json_.pop();
    return {};
  }

  auto write(std::string_view name, uint8_t value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, uint16_t value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, uint32_t value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, uint64_t value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, int8_t value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, int16_t value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, int32_t value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, int64_t value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, float value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, double value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto write(
      std::string_view name,
      std::string_view value) -> SerializeResult override {
    return write_internal(name, value);
  }

  auto data() -> std::string override {
    return root_.dump();
  }

private:
  template <class T>
  auto write_internal(std::string_view name, T value) -> SerializeResult {
    (*json_.top())[name] = value;
    return {};
  }

  std::stack<nlohmann::json*> json_;
  nlohmann::json root_;
};

struct JsonReader {
  JsonReader(nlohmann::json j)
    : root_(std::move(j)) {
    json_.push(&root_);
  }

  auto enter(std::string_view name) -> SerializeResult {
    auto& top = *json_.top();
    auto& next = top.at(name);
    json_.push(&next);
    return {};
  }

  auto leave(std::string_view name) -> SerializeResult {
    json_.pop();
    return {};
  }

  template <class T>
  auto read(std::string_view name, T& value) -> SerializeResult {
    auto& top = *json_.top();
    value = top[name];
    return {};
  }

private:
  std::stack<nlohmann::json*> json_;
  nlohmann::json root_;
};

std::shared_ptr<ISerializeWriter> createJsonWriter() {
  return std::make_shared<JsonWriter>();
}
}

struct B {
  int a;

  auto serialize(ewok::shared::TSerializeWriter auto& writer) const {
    return writer.write("a", a);
  }

  auto deserialize(ewok::shared::TSerializeReader auto& reader) {
    return reader.read("a", a);
  }
};

template <>
struct ewok::shared::IsCustomSerializable<B> : std::true_type {
};;

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

void test() {
  A a{1, 2, "3", {42}, {6, 7}};

  ewok::shared::JsonWriter writer;
  [[maybe_unused]] auto r = ewok::shared::Serializer::serialize(writer, a);
  assert(!!r);

  std::cout << writer.data() << std::endl;

  nlohmann::json j = nlohmann::json::parse(writer.data());
  std::cout << j.dump() << std::endl;

  ewok::shared::JsonReader reader(j);
  A a2;
  ewok::shared::Serializer::deserialize(reader, a2);
  assert(a2.a == a.a);
  assert(a2.f == a.f);
  assert(a2.s == a.s);
  assert(a2.b.a == a.b.a);
  assert(a2.v.x == a2.v.x);
  assert(a2.v.y == a2.v.y);
}