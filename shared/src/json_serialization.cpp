/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shared/serialization.h"

#include <string>
#include <stack>

#include "nlohmann/json.hpp"

namespace ewok::shared::serialization {
struct JsonWriter final : Writer<JsonWriter> {
  JsonWriter()
    : root_({}) {
    json_.push(&root_);
  }

  auto array(std::string_view name, size_t count) -> Result override {
    auto& top = *json_.top();
    auto& next = top[name] = nlohmann::json::array();
    json_.push(&next);
    return {};
  }

  auto enter(std::string_view name) -> Result override {
    if (auto& top = *json_.top(); top.is_array()) {
      top.push_back(nlohmann::json::object());
      auto& next = top.back();
      json_.push(&next);
    } else {
      auto& next = top[name];
      json_.push(&next);
    }
    return {};
  }

  auto leave(std::string_view name) -> Result override {
    json_.pop();
    return {};
  }

  void reset() override {
    json_ = {};
    root_ = {};
    json_.push(&root_);
  }

  auto data() -> std::string override {
    return root_.dump();
  }

  /// Template method, this is called by the various write methods in Writer<T>
  auto write(std::string_view name, auto value) -> Result {
    if (auto& top = *json_.top(); top.is_array()) {
      top.push_back(value);
    } else {
      top[name] = value;
    }
    return {};
  }

  std::stack<nlohmann::json*> json_;
  nlohmann::json root_;
};

struct JsonReader final : Reader<JsonReader> {
  explicit JsonReader(std::string const& jsonStr)
    : root_(nlohmann::json::parse(jsonStr)) {
    json_.emplace(&root_, SIZE_MAX);
  }

  auto array(std::string_view name, size_t& count) -> Result override {
    auto& top = *json_.top().first;
    if (!top.contains(name)) {
      return std::unexpected{Error::FieldNotFound};
    }

    auto& next = top.at(name);
    if (!next.is_array()) {
      return std::unexpected{Error::InvalidFormat};
    }

    json_.emplace(&next, 0);
    count = next.size();
    return {};
  }

  auto enter(std::string_view name) -> Result override {
    if (auto& top = *json_.top().first; top.is_array()) {
      auto& next = top.at(json_.top().second++);
      if (!next.is_object()) {
        return std::unexpected{Error::InvalidFormat};
      }

      json_.emplace(&next, SIZE_MAX);
    } else {
      if (!top.contains(name)) {
        return std::unexpected{Error::FieldNotFound};
      }

      auto& next = top.at(name);
      if (!next.is_object()) {
        return std::unexpected{Error::InvalidFormat};
      }
      json_.emplace(&next, SIZE_MAX);
    }

    return {};
  }

  auto leave(std::string_view name) -> Result override {
    json_.pop();
    return {};
  }

  /// Template method, this is called by the various read methods in Reader<T>
  auto read(std::string_view name, auto& value) -> Result {
    if (auto& top = *json_.top().first; top.is_array()) {
      value = top.at(json_.top().second++);
    } else {
      if (!top.contains(name)) {
        return std::unexpected{Error::FieldNotFound};
      }

      value = top.at(name);
    }
    return {};
  }

  std::stack<std::pair<nlohmann::json*, size_t>> json_;
  nlohmann::json root_;
};

std::shared_ptr<IWriter> createJsonWriter() {
  return std::make_shared<JsonWriter>();
}

std::shared_ptr<IReader> createJsonReader(std::string const& json) {
  return std::make_shared<JsonReader>(json);
}
}
