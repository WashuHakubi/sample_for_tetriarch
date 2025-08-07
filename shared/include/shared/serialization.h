/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace ewok::shared::serialization {
enum class Error {
  None,
  FieldNotFound,
  InvalidFormat,
};

using Result = std::expected<void, Error>;

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

  virtual auto data() -> std::string = 0;
};

/// Utility class that forwards calls to a derived type without requiring the derived type to implement all the methods
/// of IWriter.
template <class TDerived>
struct Writer : IWriter {
  auto write(std::string_view name, uint8_t value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }

  auto write(std::string_view name, uint16_t value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }

  auto write(std::string_view name, uint32_t value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }

  auto write(std::string_view name, uint64_t value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }

  auto write(std::string_view name, int8_t value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }

  auto write(std::string_view name, int16_t value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }

  auto write(std::string_view name, int32_t value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }

  auto write(std::string_view name, int64_t value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }

  auto write(std::string_view name, float value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }

  auto write(std::string_view name, double value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }

  auto write(std::string_view name, std::string_view value) -> Result override {
    return static_cast<TDerived*>(this)->write(name, value);
  }
};

/// Base class for deserializing fields
struct IReader {
  virtual ~IReader() = default;

  virtual auto array(std::string_view name, size_t& count) -> Result = 0;

  virtual auto enter(std::string_view name) -> Result = 0;

  virtual auto leave(std::string_view name) -> Result = 0;

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
};

/// Utility class that forwards calls to a derived type without requiring the derived type to implement all the methods
/// of IReader.
template <class TDerived>
struct Reader : IReader {
  auto read(std::string_view name, uint8_t& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }

  auto read(std::string_view name, uint16_t& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }

  auto read(std::string_view name, uint32_t& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }

  auto read(std::string_view name, uint64_t& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }

  auto read(std::string_view name, int8_t& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }

  auto read(std::string_view name, int16_t& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }

  auto read(std::string_view name, int32_t& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }

  auto read(std::string_view name, int64_t& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }

  auto read(std::string_view name, float& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }

  auto read(std::string_view name, double& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }

  auto read(std::string_view name, std::string& value) -> Result override {
    return static_cast<TDerived*>(this)->read(name, value);
  }
};

std::shared_ptr<IWriter> createJsonWriter();
std::shared_ptr<IReader> createJsonReader(std::string const& json);

/// Used in place of IWriter. This allows the code to know the concrete writer and elide the virtual function
/// calls entirely.
template <class T>
concept TSerializeWriter = std::is_base_of_v<IWriter, T>;

/// Used in place of IReader. This allows the code to know the concrete reader and elide the virtual function
/// calls entirely.
template <class T>
concept TSerializeReader = std::is_base_of_v<IReader, T>;

/// Specialize this for types that have a <tt>Result serialize(TSerializeWriter auto& writer)</tt> method.
template <class T>
struct CustomSerializable : std::false_type {
};

namespace detail {
/// Checks if we have a valid member tuple
template <class T>
struct IsMemberTuple : std::false_type {
};

template <class... Ts>
struct IsMemberTuple<std::tuple<std::pair<char const*, Ts>...>> : std::true_type {
};

template <class T>
concept TMemberTuple = IsMemberTuple<T>::value;

/// Checks if we have a static serializeMembers method that returns a valid tuple.
/// We expect this to return something like:
/// @code
/// static auto serializeMembers() {
///   return std::make_tuple(
///     std::make_pair("a", &A::a),
///   );
/// }
/// @endcode
template <class T>
concept HasSerializeMembers = requires { { T::serializeMembers() } -> TMemberTuple; };

/// Checks if a type is either a custom serializable type or has a valid serializeMembers method.
template <class T>
concept TSerializable = CustomSerializable<T>::value || HasSerializeMembers<T>;

/// Utility template to get the type from a member pointer
template <class T>
struct MemberType;

template <class C, class T>
struct MemberType<T C::*> {
  using type = std::remove_cvref_t<T>;
};

template <class T>
struct ArrayHandler : std::false_type {
};

template <class T>
struct ArrayHandler<std::vector<T>> : std::true_type {
  using item_type = T;

  static auto size(std::vector<T> const& v) { return v.size(); }
  static auto at(std::vector<T> const& v, size_t idx) { return v[idx]; }
  static auto reserve(std::vector<T>& v, size_t size) { return v.reserve(size); }
  static auto push(std::vector<T>& v, T&& i) { v.push_back(std::forward<T>(i)); }
};
}

/// Serializes @c value to @c writer
auto serialize(
    TSerializeWriter auto& writer,
    detail::TSerializable auto const& value) -> Result;

/// Deserializes from @c reader into @c value
auto deserialize(
    TSerializeReader auto& reader,
    detail::TSerializable auto& value) -> Result;

namespace detail {
auto deserializeItem(TSerializeReader auto& reader, std::string_view name, auto& value) -> Result {
  using MemberType = std::remove_cvref_t<decltype(value)>;

  if constexpr (CustomSerializable<MemberType>::value || detail::HasSerializeMembers<MemberType>) {
    // If the member is a custom serializable type or supports serializeMembers() then deserialize it recursively
    auto r = reader.enter(name);
    if (!r) {
      return r;
    }

    r = deserialize(reader, value);
    if (!r) {
      return r;
    }

    r = reader.leave(name);
    if (!r) {
      return r;
    }
  } else if constexpr (ArrayHandler<MemberType>::value) {
    size_t count{};
    auto r = reader.array(name, count);
    if (!r) {
      return r;
    }

    ArrayHandler<MemberType>::reserve(value, count);
    for (size_t i = 0; i < count; ++i) {
      typename ArrayHandler<MemberType>::item_type item;
      r = deserializeItem(reader, name, item);
      if (!r) {
        return r;
      }

      ArrayHandler<MemberType>::push(value, std::move(item));
    }

    r = reader.leave(name);
    if (!r) {
      return r;
    }
  } else {
    // Otherwise we expect the serialize writer to handle it (as it should be a primitive)
    auto r = reader.read(name, value);
    if (!r) {
      return r;
    }
  }

  return {};
}

auto serializeItem(TSerializeWriter auto& writer, std::string_view name, auto const& value) -> Result {
  using MemberType = std::remove_cvref_t<decltype(value)>;

  if constexpr (CustomSerializable<MemberType>::value || detail::HasSerializeMembers<MemberType>) {
    // If the member is a custom serializable type or supports serializeMembers() then serialize it recursively
    auto r = writer.enter(name);
    if (!r) {
      return r;
    }

    r = serialize(writer, value);
    if (!r) {
      return r;
    }

    r = writer.leave(name);
    if (!r) {
      return r;
    }
  } else if constexpr (ArrayHandler<MemberType>::value) {
    size_t count = ArrayHandler<MemberType>::size(value);
    auto r = writer.array(name, count);
    if (!r) {
      return r;
    }

    for (size_t i = 0; i < count; ++i) {
      r = serializeItem(writer, name, ArrayHandler<MemberType>::at(value, i));
      if (!r) {
        return r;
      }
    }

    r = writer.leave(name);
    if (!r) {
      return r;
    }
  } else {
    // Otherwise we expect the serialize writer to handle it (as it should be a primitive)
    auto r = writer.write(name, value);
    if (!r) {
      return r;
    }
  }

  return {};
}

/// Deserialize the Ith member of the tuple of member pointers.
template <size_t N, size_t I>
auto deserializeMember(
    TSerializeReader auto& reader,
    TSerializable auto& value,
    auto const& memberPtrTuple) -> Result {
  if constexpr (I < N) {
    auto [name, ptr] = std::get<I>(memberPtrTuple);

    auto r = deserializeItem(reader, name, value.*ptr);
    if (!r) {
      return r;
    }

    return deserializeMember<N, I + 1>(reader, value, memberPtrTuple);
  }
  return {};
}

auto deserializeMembers(
    TSerializeReader auto& reader,
    TSerializable auto& value,
    auto const& memberPtrTuple) {
  // Get the number of name/member function pointer pairs in the tuple
  constexpr auto MemberCount = std::tuple_size_v<std::remove_cvref_t<decltype(memberPtrTuple)>>;
  return deserializeMember<MemberCount, 0>(
      reader,
      value,
      memberPtrTuple);
}

/// Serialize the Ith member of the tuple of member pointers.
template <size_t N, size_t I>
auto serializeMember(
    TSerializeWriter auto& writer,
    TSerializable auto const& value,
    auto const& memberPtrTuple) -> Result {
  if constexpr (I < N) {
    auto [name, ptr] = std::get<I>(memberPtrTuple);

    auto r = serializeItem(writer, name, value.*ptr);
    if (!r) {
      return r;
    }

    return serializeMember<N, I + 1>(writer, value, memberPtrTuple);
  }
  return {};
}

auto serializeMembers(
    TSerializeWriter auto& writer,
    TSerializable auto const& value,
    auto const& memberPtrTuple) {
  // Get the number of name/member function pointer pairs in the tuple
  constexpr auto MemberCount = std::tuple_size_v<std::remove_cvref_t<decltype(memberPtrTuple)>>;
  // Start serializing the members from the first one
  return serializeMember<MemberCount, 0>(
      writer,
      value,
      memberPtrTuple);
}
} // namespace detail

[[nodiscard]] auto serialize(
    TSerializeWriter auto& writer,
    detail::TSerializable auto const& value) -> Result {
  using CustomSerializable = CustomSerializable<std::remove_cvref_t<decltype(value)>>;
  if constexpr (CustomSerializable::value) {
    // Custom serializable types should serialize themselves.
    return CustomSerializable::serialize(writer, value);
  } else {
    // Otherwise serialize each member
    auto members = value.serializeMembers();
    return detail::serializeMembers(writer, value, members);
  }
}

[[nodiscard]] auto deserialize(
    TSerializeReader auto& reader,
    detail::TSerializable auto& value) -> Result {
  using CustomSerializable = CustomSerializable<std::remove_cvref_t<decltype(value)>>;
  if constexpr (CustomSerializable::value) {
    // Custom serializable types should deserialize themselves.
    return CustomSerializable::deserialize(reader, value);
  } else {
    auto members = value.serializeMembers();
    return detail::deserializeMembers(reader, value, members);
  }
}
}