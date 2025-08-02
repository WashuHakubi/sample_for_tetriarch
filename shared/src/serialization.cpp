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

namespace ewok::shared::serialization {
struct JsonWriter : IWriter {
  JsonWriter()
    : root_({}) {
    json_.push(&root_);
  }

  auto enter(std::string_view name) -> Result override {
    auto& top = *json_.top();
    auto& next = top[name];
    json_.push(&next);
    return {};
  }

  auto leave(std::string_view name) -> Result override {
    json_.pop();
    return {};
  }

  auto write(std::string_view name, uint8_t value) -> Result override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, uint16_t value) -> Result override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, uint32_t value) -> Result override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, uint64_t value) -> Result override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, int8_t value) -> Result override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, int16_t value) -> Result override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, int32_t value) -> Result override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, int64_t value) -> Result override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, float value) -> Result override {
    return write_internal(name, value);
  }

  auto write(std::string_view name, double value) -> Result override {
    return write_internal(name, value);
  }

  auto write(
      std::string_view name,
      std::string_view value) -> Result override {
    return write_internal(name, value);
  }

  auto data() -> std::string override {
    return root_.dump();
  }

private:
  template <class T>
  auto write_internal(std::string_view name, T value) -> Result {
    (*json_.top())[name] = value;
    return {};
  }

  std::stack<nlohmann::json*> json_;
  nlohmann::json root_;
};

struct JsonReader : IReader {
  explicit JsonReader(std::string const& jsonStr)
    : root_(nlohmann::json::parse(jsonStr)) {
    json_.push(&root_);
  }

  auto enter(std::string_view name) -> Result override {
    auto& top = *json_.top();
    if (!top.contains(name)) {
      return std::unexpected{Error::FieldNotFound};
    }

    auto& next = top.at(name);
    if (!next.is_object()) {
      return std::unexpected{Error::InvalidFormat};
    }
    json_.push(&next);
    return {};
  }

  auto leave(std::string_view name) -> Result override {
    json_.pop();
    return {};
  }

  auto read(std::string_view name, uint8_t& value) -> Result override {
    return read_internal(name, value);
  }

  auto read(std::string_view name, uint16_t& value) -> Result override {
    return read_internal(name, value);
  }

  auto read(std::string_view name, uint32_t& value) -> Result override {
    return read_internal(name, value);
  }

  auto read(std::string_view name, uint64_t& value) -> Result override {
    return read_internal(name, value);
  }

  auto read(std::string_view name, int8_t& value) -> Result override {
    return read_internal(name, value);
  }

  auto read(std::string_view name, int16_t& value) -> Result override {
    return read_internal(name, value);
  }

  auto read(std::string_view name, int32_t& value) -> Result override {
    return read_internal(name, value);
  }

  auto read(std::string_view name, int64_t& value) -> Result override {
    return read_internal(name, value);
  }

  auto read(std::string_view name, float& value) -> Result override {
    return read_internal(name, value);
  }

  auto read(std::string_view name, double& value) -> Result override {
    return read_internal(name, value);
  }

  auto read(std::string_view name, std::string& value) -> Result override {
    return read_internal(name, value);
  }

private:
  template <class T>
  auto read_internal(std::string_view name, T& value) -> Result {
    auto& top = *json_.top();
    if (!top.contains(name)) {
      return std::unexpected{Error::FieldNotFound};
    }

    value = top.at(name);
    return {};
  }

  std::stack<nlohmann::json*> json_;
  nlohmann::json root_;
};

std::shared_ptr<IWriter> createJsonWriter() {
  return std::make_shared<JsonWriter>();
}

std::shared_ptr<IReader> createJsonReader(std::string const& json) {
  return std::make_shared<JsonReader>(json);
}
}
