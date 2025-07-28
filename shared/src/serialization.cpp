/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shared/serialization.h"

#include <iostream>
#include <string>

struct CoutWriter {
  auto enter(std::string_view name) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "enter " << name << "\n";
    return {};
  }

  auto leave(std::string_view name) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "leave " << name << "\n";
    return {};
  }

  auto write(std::string_view name, uint8_t value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }

  auto write(std::string_view name, uint16_t value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }

  auto write(std::string_view name, uint32_t value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }

  auto write(std::string_view name, uint64_t value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }

  auto write(std::string_view name, int8_t value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }

  auto write(std::string_view name, int16_t value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }

  auto write(std::string_view name, int32_t value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }

  auto write(std::string_view name, int64_t value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }

  auto write(std::string_view name, float value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }

  auto write(std::string_view name, double value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }

  auto write(
      std::string_view name,
      std::string_view value) -> std::expected<void, ewok::shared::SerializeError> {
    std::cout << "write " << name << " " << value << "\n";
    return {};
  }
};

struct B {
  int a;

  auto serialize(ewok::shared::TSerializeWriter auto& writer) const {
    return writer.write("a", a);
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
  B b{3};

  CoutWriter writer;
  ewok::shared::Serializer::serialize(writer, a);

  ewok::shared::Serializer::serialize(writer, b);
}