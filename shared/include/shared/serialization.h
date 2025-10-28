/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <unistd.h>

#include <array>
#include <cassert>
#include <memory>
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

  virtual void to_buffer(std::vector<char>& buffer) const = 0;
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

/// Checks if there exists a serialize overload that can handle T
template <class T>
concept has_serialize = requires(writer& writer, std::string_view name, T const& value) {
  serialize(writer, name, value);
};

/// Checks if there exists a serialize overload that can handle a compressed T
template <class T>
concept has_compressed_serialize = requires(writer& writer, std::string_view name, T const& value) {
  serialize(writer, name, value, attrs::compress);
};

/// Checks if there exists a deserialize overload that can handle T
template <class T>
concept has_deserialize = requires(reader& reader, std::string_view name, T& value) {
  deserialize(reader, name, value);
};

/// Checks if there exists a deserialize overload that can handle a compressed T
template <class T>
concept has_compressed_deserialize = requires(reader& reader, std::string_view name, T& value) {
  deserialize(reader, name, value, attrs::compress);
};

template <class T>
constexpr bool is_compressed_serializable() {
  if constexpr (has_compressed_serialize<T> && has_compressed_deserialize<T>) {
    return true;
  } else if constexpr (
      (has_compressed_serialize<T> && !has_compressed_deserialize<T>)
      // Must have both or neither.
      || (!has_compressed_serialize<T> && has_compressed_deserialize<T>)) {
    static_assert(false, "Compressed serialization requires both a deserialize and serialize overload.");
  }

  return false;
}

/// Serializes all integral primitives (bool, int, unsigned, etc.)
void serialize(writer& writer, std::string_view name, std::integral auto const& value) {
  writer.write(name, value);
}

/// Serialize compressed 16, 32, or 64-bit unsigned integers
template <class T>
  requires(contains_v<T, std::tuple<uint16_t, uint32_t, uint64_t>>)
void serialize(writer& writer, std::string_view name, T const& value, attrs::compress_tag) {
  writer.write_compressed(name, value);
}

/// Serialize floating point primitives (float, double, long double).
void serialize(writer& writer, std::string_view name, std::floating_point auto const& value) {
  writer.write(name, value);
}

/// Serialize string views
inline void serialize(writer& writer, std::string_view name, std::string_view const& value) {
  writer.write(name, value);
}

/// Serialize reflect<T> specializations
void serialize(writer& writer, std::string_view, reflectable auto const& value) {
  using reflected_type = std::decay_t<decltype(value)>;
  auto members = reflect<reflected_type>::members();

  ew::apply(
      [&value, &writer]<typename Tuple>(Tuple const& member_tuple) {
        std::string_view member_name = std::get<0>(member_tuple);
        auto member_ptr = std::get<1>(member_tuple);

        using member_type = std::decay_t<decltype(value.*member_ptr)>;
        if (reflectable<member_type>) {
          writer.begin_object(member_name);
          serialize(writer, member_name, value.*member_ptr);
          writer.end_object();
        } else if constexpr (contains_v<attrs::compress_tag, Tuple> && is_compressed_serializable<member_type>()) {
          serialize(writer, member_name, value.*member_ptr, attrs::compress);
        } else {
          serialize(writer, member_name, value.*member_ptr);
        }
      },
      members);
}

/// Serialize arrays of T
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

/// Serialize arrays of T forwarding the compression flag.
template <class T, size_t N>
void serialize(writer& writer, std::string_view name, std::array<T, N> const& value, attrs::compress_tag) {
  writer.begin_array(name, N);
  for (auto&& v : value) {
    if constexpr (reflect<T>::value) {
      writer.begin_object("");
    }

    if constexpr (is_compressed_serializable<T>()) {
      serialize(writer, "", static_cast<T const&>(v), attrs::compress);
    } else {
      serialize(writer, "", static_cast<T const&>(v));
    }

    if constexpr (reflect<T>::value) {
      writer.end_object();
    }
  }
  writer.end_array();
}

/// Serialize vectors of T.
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

/// Serialize vectors of T forwarding the compression flag.
template <class T, class Alloc>
void serialize(writer& writer, std::string_view name, std::vector<T, Alloc> const& value, attrs::compress_tag) {
  writer.begin_array(name, value.size());
  for (auto&& v : value) {
    if constexpr (reflect<T>::value) {
      writer.begin_object("");
    }

    if constexpr (is_compressed_serializable<T>()) {
      serialize(writer, "", static_cast<T const&>(v), attrs::compress);
    } else {
      serialize(writer, "", static_cast<T const&>(v));
    }

    if constexpr (reflect<T>::value) {
      writer.end_object();
    }
  }
  writer.end_array();
}

template <class T>
void serialize(writer& writer, T const& value) {
  if constexpr (has_serialize<T>) {
    serialize(writer, "", value);
  } else {
    static_assert(false, "Expected primitive type, reflect<T> specialization, or serialize overload");
  }
}

/// Deserializes all integral primitives (bool, int, unsigned, etc.)
void deserialize(reader& reader, std::string_view name, std::integral auto& value) {
  reader.read(name, value);
}

/// Deserialize compressed 16, 32, or 64-bit unsigned integers
template <class T>
  requires(contains_v<T, std::tuple<uint16_t, uint32_t, uint64_t>>)
void deserialize(reader& reader, std::string_view name, T& value, attrs::compress_tag) {
  reader.read_compressed(name, value);
}

/// Deserializes all floating point primitives (float, double, long double).
void deserialize(reader& reader, std::string_view name, std::floating_point auto& value) {
  reader.read(name, value);
}

/// Deserializes strings
inline void deserialize(reader& reader, std::string_view name, std::string& value) {
  reader.read(name, value);
}

/// Deserializes reflect<T> specializations
void deserialize(reader& reader, std::string_view name, reflectable auto& value) {
  using reflected_type = std::decay_t<decltype(value)>;
  auto members = reflect<reflected_type>::members();

  ew::apply(
      [&value, &reader]<typename Tuple>(Tuple const& member_tuple) {
        std::string_view member_name = std::get<0>(member_tuple);
        auto member_ptr = std::get<1>(member_tuple);

        using member_type = std::decay_t<decltype(value.*member_ptr)>;
        if (reflectable<member_type>) {
          reader.begin_object(member_name);
          deserialize(reader, member_name, value.*member_ptr);
          reader.end_object();
        } else if constexpr (contains_v<attrs::compress_tag, Tuple> && is_compressed_serializable<member_type>()) {
          deserialize(reader, member_name, value.*member_ptr, attrs::compress);
        } else {
          deserialize(reader, member_name, value.*member_ptr);
        }
      },
      members);
}

/// Delegating overload. This is here for the static assertion.
template <class T>
void deserialize(reader& reader, std::string_view name, T& value) {
  if constexpr (has_deserialize<T>) {
    deserialize(reader, name, value);
  } else {
    static_assert(false, "Expected primitive type, reflect<T> specialization, or deserialize overload");
  }
}

/// Deserialize arrays of T.
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

/// Deserialize arrays of T forwarding the compression flag.
template <class T, size_t N>
void deserialize(reader& reader, std::string_view name, std::array<T, N>& value, attrs::compress_tag) {
  size_t n;
  reader.begin_array(name, n);
  assert(n == N);

  for (size_t i = 0; i < N; ++i) {
    if constexpr (reflect<T>::value) {
      reader.begin_object("");
    }

    if constexpr (is_compressed_serializable<T>()) {
      deserialize(reader, "", value[i], attrs::compress);
    } else {
      deserialize(reader, "", value[i]);
    }

    if constexpr (reflect<T>::value) {
      reader.end_object();
    }
  }
  reader.end_array();
}

/// Deserialize vectors of T.
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

    // It would be nice if we could re-use the std::array code here, but std::vector<bool> ruins everything.
    value.push_back(std::move(v));

    if constexpr (reflect<T>::value) {
      reader.end_object();
    }
  }
  reader.end_array();
}

/// Deserialize vectors of T forwarding the compression flag.
template <class T, class Alloc>
void deserialize(reader& reader, std::string_view name, std::vector<T, Alloc>& value, attrs::compress_tag) {
  size_t n;
  reader.begin_array(name, n);
  value.reserve(n);

  for (size_t i = 0; i < n; ++i) {
    if constexpr (reflect<T>::value) {
      reader.begin_object("");
    }

    T v;

    if constexpr (is_compressed_serializable<T>()) {
      deserialize(reader, "", v, attrs::compress);
    } else {
      deserialize(reader, "", v);
    }

    // It would be nice if we could re-use the std::array code here, but std::vector<bool> ruins everything.
    value.push_back(std::move(v));

    if constexpr (reflect<T>::value) {
      reader.end_object();
    }
  }
  reader.end_array();
}

template <class T>
void deserialize(reader& reader, T& value) {
  deserialize(reader, "", value);
}

/// Creates a json writer, suitable for writing structured data.
std::shared_ptr<writer> create_json_writer();

/// Creates a json reader, suitable for reading structured data.
std::shared_ptr<reader> create_json_reader(std::string_view json);

/// Creates a binary writer with an internal buffer, suitable for flat serialization. This should be used for writing
/// out data where the structure of the data does not need to be preserved and should always match the compiled
/// definition.
std::shared_ptr<writer> create_binary_writer();

/// Creates a binary writer, suitable for flat serialization. This should be used for writing out data where the
/// structure of the data does not need to be preserved and should always match the compiled definition. This emits the
/// serialized data to the passed buffer
std::shared_ptr<writer> create_binary_writer(std::vector<char>& buffer);

/// Creates a binary reader, suitable for flat deserialization. This should be used for reading data where the structure
/// of the data does not need to be preserved and where the data should match the compiled layout.
std::shared_ptr<reader> create_binary_reader(std::span<char const> buffer);
} // namespace ew