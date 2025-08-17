/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

namespace ewok::shared::serialization {
/// Base class for serializing fields.
struct IWriter {
  virtual ~IWriter() = default;

  /**
   * Begins an array. Count is expected to be the number of elements in the array. All calls to @c write, up to @c
   * count, should write an entry into the array.
   */
  virtual auto array(std::string_view name, size_t count) -> Result = 0;

  /**
   * Begins an object, each call to write should write a named value into the object
   */
  virtual auto enter(std::string_view name) -> Result = 0;

  /**
   * Ends the current array or object.
   */
  virtual auto leave(std::string_view name) -> Result = 0;

  virtual auto write(std::string_view name, bool value) -> Result = 0;
  virtual auto write(std::string_view name, uint8_t value) -> Result = 0;
  virtual auto write(std::string_view name, uint16_t value) -> Result = 0;
  virtual auto write(std::string_view name, uint32_t value) -> Result = 0;
  virtual auto write(std::string_view name, uint64_t value) -> Result = 0;
  virtual auto write(std::string_view name, int8_t value) -> Result = 0;
  virtual auto write(std::string_view name, int16_t value) -> Result = 0;
  virtual auto write(std::string_view name, int32_t value) -> Result = 0;
  virtual auto write(std::string_view name, int64_t value) -> Result = 0;
  virtual auto write(std::string_view name, float value) -> Result = 0;
  virtual auto write(std::string_view name, double value) -> Result = 0;
  virtual auto write(std::string_view name, std::string_view value) -> Result = 0;

  virtual void reset() = 0;

  virtual auto data() -> std::string = 0;
};

struct IBinWriter : IWriter {
  [[nodiscard]] virtual auto fieldMapping() const ->
    std::unordered_map<
      std::tuple<std::string, int>,
      std::tuple<size_t, BinFieldType>> const& = 0;
};

/// Utility class that forwards calls to a derived type without requiring the derived type to implement all the methods
/// of IWriter.
template <class TDerived, class Interface = IWriter>
struct Writer : Interface {
  auto write(std::string_view name, bool value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, uint8_t value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, uint16_t value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, uint32_t value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, uint64_t value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, int8_t value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, int16_t value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, int32_t value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, int64_t value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, float value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, double value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }

  auto write(std::string_view name, std::string_view value) -> Result override {
    return static_cast<TDerived*>(this)->writeInternal(name, value);
  }
};

std::shared_ptr<IWriter> createJsonWriter(bool prettyPrint = false);
std::shared_ptr<IBinWriter> createBinWriter(std::string& buffer, bool trackFields = false);
}