/*
 * Copyright (c) 2026 Sean Kent.
 * All Rights Reserved.
 *
 */

#pragma once

#include "serialization.h"

#include <stack>
#include <nlohmann/json.hpp>

namespace sft {
/// JSON serialization implementation of the IWriter interface. Builds a
/// nlohmann::json tree incrementally via beginObject/endObject and
/// beginArray/endArray nesting, and provides typed write() methods for
/// all supported scalar types. Root must always be an object; individual
/// write() calls populate named fields or append to the current array.
struct JsonWriter final : IWriter {
  JsonWriter();

  void beginArray(std::string_view name, size_t len) override;

  void beginObject(std::string_view name) override;

  void endArray() override;

  void endObject() override;

  void write(std::string_view name, bool v) override;

  void write(std::string_view name, char v) override;

  void write(std::string_view name, int8_t v) override;

  void write(std::string_view name, int16_t v) override;

  void write(std::string_view name, int32_t v) override;

  void write(std::string_view name, int64_t v) override;

  void write(std::string_view name, uint8_t v) override;

  void write(std::string_view name, uint16_t v) override;

  void write(std::string_view name, uint32_t v) override;

  void write(std::string_view name, uint64_t v) override;

  void write(std::string_view name, float v) override;

  void write(std::string_view name, double v) override;

  void write(std::string_view name, std::string_view v) override;

  /// Serialize the entire JSON tree into a formatted string.
  /// @param[out] out Output buffer (overwritten). Uses 2-space indentation.
  void dump(std::string& out) const;

 private:
  nlohmann::json root_; ///< Root JSON object.
  std::stack<nlohmann::json*> nests_; ///< Stack of current nesting contexts.
};

/// JSON deserialization implementation of the IReader interface. Parses a
/// JSON string into a nlohmann::json tree and provides typed read() methods
/// that extract values sequentially. When traversing arrays, elements are
/// consumed in order by advancing an internal index; object fields are
/// looked up by name. Missing entries default-initialize the output.
struct JsonReader final : IReader {
  JsonReader(std::vector<std::byte> const& data)
      : JsonReader(std::string_view{reinterpret_cast<char const*>(data.data()), data.size()}) {}

  JsonReader(std::vector<char> const& data) : JsonReader(std::string_view{data.data(), data.size()}) {}

  /// Parse a JSON string and initialize the reader.
  /// @param json JSON-formatted string to parse
  JsonReader(std::string_view json);

  void beginArray(std::string_view name, size_t& len) override;

  void beginObject(std::string_view name) override;

  void endArray() override;

  void endObject() override;

  auto hasKey(std::string_view name) -> bool override;

  void read(std::string_view name, bool& v) override;

  void read(std::string_view name, char& v) override;

  void read(std::string_view name, int8_t& v) override;

  void read(std::string_view name, int16_t& v) override;

  void read(std::string_view name, int32_t& v) override;

  void read(std::string_view name, int64_t& v) override;

  void read(std::string_view name, uint8_t& v) override;

  void read(std::string_view name, uint16_t& v) override;

  void read(std::string_view name, uint32_t& v) override;

  void read(std::string_view name, uint64_t& v) override;

  void read(std::string_view name, float& v) override;

  void read(std::string_view name, double& v) override;

  void read(std::string_view name, std::string& v) override;

 private:
  nlohmann::json root_; ///< Parsed root JSON value.
  std::stack<std::tuple<nlohmann::json*, size_t>> nests_; ///< Stack of (current context, array index) pairs.
};
} // namespace sft
