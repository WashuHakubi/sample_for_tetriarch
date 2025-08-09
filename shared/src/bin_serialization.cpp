/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shared/serialization.h"

#include <cstring>
#include <stack>
#include <unordered_map>
#include <tuple>

namespace ewok::shared::serialization {
/// Writes out data linearly to a binary buffer. This can optionally keep track of the fields as it writes them.
class BinWriter : public Writer<BinWriter, IBinWriter> {
public:
  BinWriter(std::string& buffer, bool trackFields)
    : trackFields_(trackFields),
      data_(&buffer) {
    if (trackFields_) {
      stack_.push(false);
    }
  }

  auto array(std::string_view name, size_t count) -> Result override {
    expect(name, BinFieldType::Array, true);
    append(static_cast<uint32_t>(count));
    return {};
  }

  auto enter(std::string_view name) -> Result override {
    expect(name, BinFieldType::Object, true);
    return {};
  }

  auto leave(std::string_view name) -> Result override {
    if (trackFields_) {
      stack_.pop();
    }
    return {};
  }

  auto write(std::string_view name, auto value) -> Result {
    expect(name, BinFieldType::Value);
    append(value);
    return {};
  }

  auto write(std::string_view name, bool value) -> Result override {
    expect(name, BinFieldType::Value);
    append(static_cast<char>(value));
    return {};
  }

  auto write(std::string_view name, std::string_view value) -> Result override {
    expect(name, BinFieldType::Value);
    append(static_cast<uint32_t>(value.size()));
    data_->append(value.data(), value.size());
    return {};
  }

  auto data() -> std::string override {
    return *data_;
  }

  void reset() override {
    data_->clear();
    if (trackFields_) {
      fieldOffset_ = {};
      stack_ = {};
      stack_.push(false);
      seq_ = 0;
    }
  }

  auto fieldMapping() const ->
    std::unordered_map<
      std::tuple<std::string, int>,
      std::tuple<size_t, BinFieldType>> const& override {
    return fieldOffset_;
  }

private:
  template <class T>
    requires(std::is_arithmetic_v<T>)
  void append(T value) {
    const auto bytes = reinterpret_cast<char const*>(&value);
    data_->append(bytes, bytes + sizeof(T));
  }

  void expect(std::string_view name, BinFieldType type, bool increment = false) {
    if (!trackFields_) { return; }

    [[maybe_unused]] auto [_, inserted] = fieldOffset_.emplace(
        std::make_tuple(name, seq_),
        std::make_tuple(data_->size(), type));
    // If we're in array mode then ignore duplicates. In object mode duplicates must not exist.
    assert(stack_.top() || inserted);

    if (increment) {
      stack_.push(type == BinFieldType::Array);
      // Advance the sequence number. We expect all fields for an Object type to be unique within their sequence.
      // For arrays field names may be repeated.
      ++seq_;
    }
  }

  bool trackFields_;
  std::unordered_map<std::tuple<std::string, int>, std::tuple<size_t, BinFieldType>> fieldOffset_;
  int seq_ = 0;
  std::stack<bool> stack_;

  std::string* data_;
};

/// Reads data in from a byte buffer. This can optionally keep track of the fields as it reads them.
struct BinReader : Reader<BinReader, IBinReader> {
  explicit BinReader(std::span<char> data, bool trackFields)
    : trackFields_(trackFields),
      data_(data) {
    if (trackFields_) {
      stack_.push(false);
    }
  }

  auto array(std::string_view name, size_t& count) -> Result override {
    expect(name, BinFieldType::Array, true);
    return extract<uint32_t>()
        .and_then(
            [&count](auto c) -> Result {
              count = c;
              return {};
            });
  }

  auto enter(std::string_view name) -> Result override {
    expect(name, BinFieldType::Object, true);
    return {};
  }

  auto leave(std::string_view name) -> Result override {
    if (trackFields_) {
      stack_.pop();
    }
    return {};
  }

  auto read(std::string_view name, auto& value) -> Result {
    expect(name, BinFieldType::Value);
    return extract<std::remove_cvref_t<decltype(value)>>()
        .and_then(
            [&value](auto v) -> Result {
              value = v;
              return {};
            });
  }

  auto read(std::string_view name, bool& value) -> Result override {
    expect(name, BinFieldType::Value);
    return extract<char>()
        .and_then(
            [&value](auto v) -> Result {
              value = v != 0;
              return {};
            });
  }

  auto read(std::string_view name, std::string& value) -> Result override {
    expect(name, BinFieldType::Value);
    return extract<uint32_t>()
        .and_then(
            [&value, this](auto len) -> Result {
              value.resize(len);
              if (data_.size() + readPos_ < len) {
                return std::unexpected{Error::InvalidFormat};
              }

              memcpy(value.data(), data_.data() + readPos_, len);
              readPos_ += len;
              return {};
            });
  }

  auto fieldMapping() const ->
    std::unordered_map<
      std::tuple<std::string, int>,
      std::tuple<size_t, BinFieldType>> const& override {
    return fieldOffset_;
  }

private:
  template <class T>
    requires(std::is_arithmetic_v<T>)
  std::expected<T, Error> extract() {
    T value;
    if (data_.size() + readPos_ < sizeof(T)) {
      return std::unexpected{Error::InvalidFormat};
    }

    memcpy(&value, data_.data() + readPos_, sizeof(T));
    readPos_ += sizeof(T);
    return value;
  }

  void expect(std::string_view name, BinFieldType type, bool increment = false) {
    if (!trackFields_) { return; }

    [[maybe_unused]] auto [_, inserted] = fieldOffset_.emplace(
        std::make_tuple(name, seq_),
        std::make_tuple(readPos_, type));
    // If we're in array mode then ignore duplicates. In object mode duplicates must not exist.
    assert(stack_.top() || inserted);

    if (increment) {
      stack_.push(type == BinFieldType::Array);
      ++seq_;
    }
  }

  bool trackFields_;
  std::unordered_map<std::tuple<std::string, int>, std::tuple<size_t, BinFieldType>> fieldOffset_;
  int seq_ = 0;
  std::stack<bool> stack_;

  size_t readPos_ = 0;
  std::span<char> data_;
};

std::shared_ptr<IBinWriter> createBinWriter(std::string& buffer, bool trackFields) {
  return std::make_shared<BinWriter>(buffer, trackFields);
}

std::shared_ptr<IBinReader> createBinReader(std::span<char> buffer, bool trackFields) {
  return std::make_shared<BinReader>(buffer, trackFields);
}
}