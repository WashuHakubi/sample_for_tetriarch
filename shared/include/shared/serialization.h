/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <unistd.h>

#include <array>
#include <string_view>
#include <vector>

#include <shared/reflection.h>

namespace ew {
struct writer {
  virtual ~writer() = default;

  virtual void begin_object(std::string_view name) = 0;
  virtual void end_object() = 0;

  virtual void begin_array(std::string_view name, size_t count) = 0;
  virtual void end_array() = 0;

  virtual void write(std::string_view name, bool value) = 0;

  virtual void write(std::string_view name, uint8_t value) = 0;
  virtual void write(std::string_view name, uint16_t value) = 0;
  virtual void write(std::string_view name, uint32_t value) = 0;
  virtual void write(std::string_view name, uint64_t value) = 0;

  virtual void write(std::string_view name, int8_t value) = 0;
  virtual void write(std::string_view name, int16_t value) = 0;
  virtual void write(std::string_view name, int32_t value) = 0;
  virtual void write(std::string_view name, int64_t value) = 0;

  virtual void write_compressed(std::string_view name, uint16_t value) = 0;
  virtual void write_compressed(std::string_view name, uint32_t value) = 0;
  virtual void write_compressed(std::string_view name, uint64_t value) = 0;

  virtual void write(std::string_view name, float value) = 0;
  virtual void write(std::string_view name, double value) = 0;

  virtual void write(std::string_view name, std::string_view value) = 0;
};

struct reader {
  virtual ~reader() = default;

  virtual void begin_object(std::string_view name) = 0;
  virtual void end_object() = 0;

  virtual void begin_array(std::string_view name, size_t& count) = 0;
  virtual void end_array() = 0;

  virtual void read(std::string_view name, bool& value) = 0;

  virtual void read(std::string_view name, uint8_t& value) = 0;
  virtual void read(std::string_view name, uint16_t& value) = 0;
  virtual void read(std::string_view name, uint32_t& value) = 0;
  virtual void read(std::string_view name, uint64_t& value) = 0;

  virtual void read(std::string_view name, int8_t& value) = 0;
  virtual void read(std::string_view name, int16_t& value) = 0;
  virtual void read(std::string_view name, int32_t& value) = 0;
  virtual void read(std::string_view name, int64_t& value) = 0;

  virtual void read_compressed(std::string_view name, uint16_t& value) = 0;
  virtual void read_compressed(std::string_view name, uint32_t& value) = 0;
  virtual void read_compressed(std::string_view name, uint64_t& value) = 0;

  virtual void read(std::string_view name, float& value) = 0;
  virtual void read(std::string_view name, double& value) = 0;

  virtual void read(std::string_view name, std::string& value) = 0;
};

template <class T>
void serialize(writer& writer, T const& value) {
  serialize(writer, "", value);
}

template <class T>
void serialize(writer& writer, std::string_view name, T const& value) {
  if constexpr (reflect<T>::value) {
    ew::apply(
        [&value, &writer]<typename Tuple>(Tuple const& member_tuple) {
          std::string_view name = std::get<0>(member_tuple);
          auto member_ptr = std::get<1>(member_tuple);

          // Get the bare type of the member
          using member_type = std::decay_t<decltype(value.*member_ptr)>;

          if constexpr (reflect<member_type>::value) {
            writer.begin_object(name);
            serialize(writer, name, value.*member_ptr);
            writer.end_object();
          } else if constexpr (contains_v<attrs::compress_tag, Tuple>) {
            static_assert(
                std::is_same_v<member_type, uint16_t>
                || std::is_same_v<member_type, uint32_t>
                || std::is_same_v<member_type, uint64_t>,
                "compress flag is only valid on 16, 32 or 64 bit unsigned integers.");
            writer.write_compressed(name, value.*member_ptr);
          } else if constexpr (std::is_arithmetic_v<member_type>
            || std::is_convertible_v<std::string_view, member_type>) {
            writer.write(name, value.*member_ptr);
          } else {
            serialize(writer, name, value.*member_ptr);
          }
        },
        reflect<T>::members());
  } else if constexpr (std::is_arithmetic_v<T>
    || std::is_convertible_v<std::string_view, T>) {
    writer.write(name, value);
  } else {
    static_assert(false, "Expected primitive type, reflect<T> specialization, or serialize overload");
  }
}

template <class T, size_t N>
void serialize(writer& writer, std::string_view name, std::array<T, N> const& value) {
  writer.begin_array(name, N);
  for (auto&& v : value) {
    if constexpr (reflect<T>::value) {
      writer.begin_object("");
    }
    serialize(writer, "", v);
    if constexpr (reflect<T>::value) {
      writer.end_object();
    }
  }
  writer.end_array();
}

template <class T, class Alloc>
void serialize(writer& writer, std::string_view name, std::vector<T, Alloc> const& value) {
  writer.begin_array(name, value.size());
  for (auto&& v : value) {
    if constexpr (reflect<T>::value) {
      writer.begin_object("");
    }
    serialize(writer, "", static_cast<T const&>(v));
    if constexpr (reflect<T>::value) {
      writer.end_object();
    }
  }
  writer.end_array();
}

template <class T>
void deserialize(reader& reader, std::string_view name, T& value) {
  if constexpr (reflect<T>::value) {
    ew::apply(
        [&value, &reader]<typename Tuple>(Tuple const& member_tuple) {
          std::string_view name = std::get<0>(member_tuple);
          auto member_ptr = std::get<1>(member_tuple);

          // Get the bare type of the member
          using member_type = std::decay_t<decltype(value.*member_ptr)>;

          if constexpr (reflect<member_type>::value) {
            reader.begin_object(name);
            deserialize(reader, name, value.*member_ptr);
            reader.end_object();
          } else if constexpr (contains_v<attrs::compress_tag, Tuple>) {
            static_assert(
                std::is_same_v<member_type, uint16_t> || std::is_same_v<member_type, uint32_t> ||
                std::is_same_v<member_type, uint64_t>,
                "compress flag is only valid on 16, 32 or 64 bit unsigned integers.");
            reader.read_compressed(name, value.*member_ptr);
          } else if constexpr (std::is_arithmetic_v<member_type>
            || std::is_convertible_v<std::string_view, member_type>) {
            reader.read(name, value.*member_ptr);
          } else {
            deserialize(reader, name, value.*member_ptr);
          }
        },
        reflect<T>::members());
  } else if constexpr (std::is_arithmetic_v<T>
    || std::is_convertible_v<std::string_view, T>) {
    reader.read(name, value);
  } else {
    static_assert(false, "Expected primitive type, reflect<T> specialization, or deserialize overload");
  }
}

template <class T, size_t N>
void deserialize(reader& reader, std::string_view name, std::array<T, N>& value) {
  size_t n;
  reader.begin_array(name, n);
  assert(n == N);

  for (size_t i = 0; i < N; ++i) {
    if constexpr (reflect<T>::value) {
      reader.begin_object("");
    }
    deserialize(reader, "", value[i]);
    if constexpr (reflect<T>::value) {
      reader.end_object();
    }
  }
  reader.end_array();
}

template <class T, class Alloc>
void deserialize(reader& reader, std::string_view name, std::vector<T, Alloc>& value) {
  size_t n;
  reader.begin_array(name, n);
  value.reserve(n);

  for (size_t i = 0; i < n; ++i) {
    if constexpr (reflect<T>::value) {
      reader.begin_object("");
    }

    T v;
    deserialize(reader, "", v);
    value.push_back(std::move(v));

    if constexpr (reflect<T>::value) {
      reader.end_object();
    }
  }
  reader.end_array();
}
} // namespace ew