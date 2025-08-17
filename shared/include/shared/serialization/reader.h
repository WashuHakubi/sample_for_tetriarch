/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

namespace ewok::shared::serialization {
/// Base class for deserializing fields
struct IReader {
  virtual ~IReader() = default;

  virtual auto array(std::string_view name, size_t& count) -> Result = 0;

  virtual auto enter(std::string_view name) -> Result = 0;

  virtual auto leave(std::string_view name) -> Result = 0;

  virtual auto read(std::string_view name, bool& value) -> Result = 0;
  virtual auto read(std::string_view name, uint8_t& value) -> Result = 0;
  virtual auto read(std::string_view name, uint16_t& value) -> Result = 0;
  virtual auto read(std::string_view name, uint32_t& value) -> Result = 0;
  virtual auto read(std::string_view name, uint64_t& value) -> Result = 0;
  virtual auto read(std::string_view name, int8_t& value) -> Result = 0;
  virtual auto read(std::string_view name, int16_t& value) -> Result = 0;
  virtual auto read(std::string_view name, int32_t& value) -> Result = 0;
  virtual auto read(std::string_view name, int64_t& value) -> Result = 0;
  virtual auto read(std::string_view name, float& value) -> Result = 0;
  virtual auto read(std::string_view name, double& value) -> Result = 0;
  virtual auto read(std::string_view name, std::string& value) -> Result = 0;

  virtual void reset(std::span<char const> buffer) = 0;
};

struct IBinReader : IReader {
  [[nodiscard]] virtual auto fieldMapping() const ->
    std::unordered_map<
      std::tuple<std::string, int>,
      std::tuple<size_t, BinFieldType>> const& = 0;
};

/// Utility class that forwards calls to a derived type without requiring the derived type to implement all the methods
/// of IReader.
template <class TDerived, class Interface = IReader>
struct Reader : Interface {
  auto read(std::string_view name, bool& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, uint8_t& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, uint16_t& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, uint32_t& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, uint64_t& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, int8_t& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, int16_t& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, int32_t& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, int64_t& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, float& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, double& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }

  auto read(std::string_view name, std::string& value) -> Result override {
    return static_cast<TDerived*>(this)->readInternal(name, value);
  }
};

std::shared_ptr<IReader> createJsonReader(std::span<char const> json);

inline std::shared_ptr<IReader> createJsonReader(std::string_view json) {
  return createJsonReader(std::span{json.data(), json.size()});
}

inline std::shared_ptr<IReader> createJsonReader(std::string const& json) {
  return createJsonReader(std::span{json.data(), json.size()});
}

std::shared_ptr<IBinReader> createBinReader(std::span<char> buffer, bool trackFields = false);
}