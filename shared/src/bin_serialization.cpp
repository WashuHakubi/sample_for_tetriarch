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
static const std::unordered_map<
  std::tuple<std::string, int>,
  std::tuple<size_t, BinFieldType>> s_empty;

/// Writes out data linearly to a binary buffer. This can optionally keep track of the fields as it writes them.
class BinWriter : public Writer<BinWriter, IBinWriter> {
public:
  explicit BinWriter(std::string& buffer)
    : data_(&buffer) {
  }

  auto array(std::string_view name, size_t count) -> Result override {
    append(static_cast<uint32_t>(count));
    return {};
  }

  auto enter(std::string_view name) -> Result override {
    return {};
  }

  auto leave(std::string_view name) -> Result override {
    return {};
  }

  auto write(std::string_view name, auto value) -> Result {
    append(value);
    return {};
  }

  auto write(std::string_view name, bool value) -> Result override {
    append(static_cast<char>(value));
    return {};
  }

  auto write(std::string_view name, std::string_view value) -> Result override {
    append(static_cast<uint32_t>(value.size()));
    data_->append(value.data(), value.size());
    return {};
  }

  auto data() -> std::string override {
    return *data_;
  }

  void reset() override {
    data_->clear();
  }

  [[nodiscard]] auto fieldMapping() const ->
    std::unordered_map<
      std::tuple<std::string, int>,
      std::tuple<size_t, BinFieldType>> const& override {
    return s_empty;
  }

  [[nodiscard]] auto writePos() const -> size_t {
    return data_->size();
  }

private:
  template <class T>
    requires(std::is_arithmetic_v<T>)
  void append(T value) {
    const auto bytes = reinterpret_cast<char const*>(&value);
    data_->append(bytes, bytes + sizeof(T));
  }


  std::string* data_;
};

/// Tracking variant of binary writer. Useful for debugging.
struct TrackingBinWriter : Writer<TrackingBinWriter, IBinWriter> {
  explicit TrackingBinWriter(std::string& buffer)
    : writer_(buffer) {
    stack_.push(false);
  }

  auto array(std::string_view name, size_t count) -> Result override {
    expect(name, BinFieldType::Array, true);
    return writer_.array(name, count);
  }

  auto enter(std::string_view name) -> Result override {
    expect(name, BinFieldType::Object, true);
    return writer_.enter(name);
  }

  auto leave(std::string_view name) -> Result override {
    stack_.pop();
    return writer_.leave(name);
  }

  void reset() override {
    fieldMappings_ = {};
    stack_ = {};
    stack_.push(false);
    seq_ = 0;
    return writer_.reset();
  }

  auto data() -> std::string override {
    return writer_.data();
  }

  auto fieldMapping() const ->
    std::unordered_map<
      std::tuple<std::string, int>,
      std::tuple<size_t, BinFieldType>> const& override {
    return fieldMappings_;
  }

  auto write(std::string_view name, auto value) -> Result {
    expect(name, BinFieldType::Value);
    return writer_.write(name, value);
  }

private:
  void expect(std::string_view name, BinFieldType type, bool increment = false) {
    [[maybe_unused]] auto [_, inserted] = fieldMappings_.emplace(
        std::make_tuple(name, seq_),
        std::make_tuple(writer_.writePos(), type));
    // If we're in array mode then ignore duplicates. In object mode duplicates must not exist.
    assert(stack_.top() || inserted);

    if (increment) {
      stack_.push(type == BinFieldType::Array);
      // Advance the sequence number. We expect all fields for an Object type to be unique within their sequence.
      // For arrays field names may be repeated.
      ++seq_;
    }
  }

  std::unordered_map<std::tuple<std::string, int>, std::tuple<size_t, BinFieldType>> fieldMappings_;
  int seq_ = 0;
  std::stack<bool> stack_;

  BinWriter writer_;
};

/// Reads data in from a byte buffer.
struct BinReader : Reader<BinReader, IBinReader> {
  explicit BinReader(std::span<char> data)
    : data_(data) {
  }

  auto array(std::string_view name, size_t& count) -> Result override {
    return extract<uint32_t>()
        .and_then(
            [&count](auto c) -> Result {
              count = c;
              return {};
            });
  }

  auto enter(std::string_view name) -> Result override {
    return {};
  }

  auto leave(std::string_view name) -> Result override {
    return {};
  }

  auto read(std::string_view name, auto& value) -> Result {
    return extract<std::remove_cvref_t<decltype(value)>>()
        .and_then(
            [&value](auto v) -> Result {
              value = v;
              return {};
            });
  }

  auto read(std::string_view name, bool& value) -> Result override {
    return extract<char>()
        .and_then(
            [&value](auto v) -> Result {
              value = v != 0;
              return {};
            });
  }

  auto read(std::string_view name, std::string& value) -> Result override {
    return extract<uint32_t>()
        .and_then(
            [&value, this](auto len) -> Result {
              if (data_.size() + readPos_ < len) {
                // Too little buffer remaining for this string.
                return std::unexpected{Error::InvalidFormat};
              }

              value.resize(len);
              memcpy(value.data(), data_.data() + readPos_, len);
              readPos_ += len;
              return {};
            });
  }

  [[nodiscard]] auto fieldMapping() const ->
    std::unordered_map<
      std::tuple<std::string, int>,
      std::tuple<size_t, BinFieldType>> const& override {
    return s_empty;
  }

  [[nodiscard]] auto readPos() const -> size_t {
    return readPos_;
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

  size_t readPos_ = 0;
  std::span<char> data_;
};

/// Tracking variant of binary reader. Useful for debugging.
struct TrackingBinReader : Reader<TrackingBinReader, IBinReader> {
  explicit TrackingBinReader(std::span<char> data)
    : reader_(data) {
    stack_.push(false);
  }

  auto array(std::string_view name, size_t& count) -> Result override {
    expect(name, BinFieldType::Array, true);
    return reader_.array(name, count);
  }

  auto enter(std::string_view name) -> Result override {
    expect(name, BinFieldType::Object, true);
    return {};
  }

  auto leave(std::string_view name) -> Result override {
    stack_.pop();
    return {};
  }

  auto read(std::string_view name, auto& value) -> Result {
    expect(name, BinFieldType::Value);
    return reader_.read(name, value);
  }

  auto fieldMapping() const ->
    std::unordered_map<
      std::tuple<std::string, int>,
      std::tuple<size_t, BinFieldType>> const& override {
    return fieldMappings_;
  }

private:
  void expect(std::string_view name, BinFieldType type, bool increment = false) {
    [[maybe_unused]] auto [_, inserted] = fieldMappings_.emplace(
        std::make_tuple(name, seq_),
        std::make_tuple(reader_.readPos(), type));
    // If we're in array mode then ignore duplicates. In object mode duplicates must not exist.
    assert(stack_.top() || inserted);

    if (increment) {
      stack_.push(type == BinFieldType::Array);
      ++seq_;
    }
  }

  std::unordered_map<std::tuple<std::string, int>, std::tuple<size_t, BinFieldType>> fieldMappings_;
  int seq_ = 0;
  std::stack<bool> stack_;

  BinReader reader_;
};

std::shared_ptr<IBinWriter> createBinWriter(std::string& buffer, bool trackFields) {
  if (trackFields) {
    return std::make_shared<TrackingBinWriter>(buffer);
  }
  return std::make_shared<BinWriter>(buffer);
}

std::shared_ptr<IBinReader> createBinReader(std::span<char> buffer, bool trackFields) {
  if (trackFields) {
    return std::make_shared<TrackingBinReader>(buffer);
  }
  return std::make_shared<BinReader>(buffer);
}
}