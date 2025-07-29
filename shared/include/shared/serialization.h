/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <cstdint>
#include <concepts>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
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

/// Base class which is useful for knowing the core set of members to implement
struct IWriter {
  virtual ~IWriter() = default;

  virtual auto enter(std::string_view name) -> Result = 0;
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

struct IReader {
  virtual ~IReader() = default;

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

std::shared_ptr<IWriter> createJsonWriter();
std::shared_ptr<IReader> createJsonReader(std::string const& json);

namespace detail {
template <class T, class U>
concept THasSerializeWrite = requires(T& t, std::string_view name, U u)
{
  { t.write(name, u) } -> std::same_as<Result>;
};

template <class T, class U>
concept THasSerializeRead = requires(T& t, std::string_view name, U& u)
{
  { t.read(name, u) } -> std::same_as<Result>;
};
}

/// A serialization writer must support writing all integral, floating point, and string_view types.
/// In addition, it must support an enter(name) and leave() methods.
template <class T>
concept TSerializeWriter =
    detail::THasSerializeWrite<T, uint8_t> &&
    detail::THasSerializeWrite<T, uint16_t> &&
    detail::THasSerializeWrite<T, uint32_t> &&
    detail::THasSerializeWrite<T, uint64_t> &&
    detail::THasSerializeWrite<T, int8_t> &&
    detail::THasSerializeWrite<T, int16_t> &&
    detail::THasSerializeWrite<T, int32_t> &&
    detail::THasSerializeWrite<T, int64_t> &&
    detail::THasSerializeWrite<T, float> &&
    detail::THasSerializeWrite<T, double> &&
    detail::THasSerializeWrite<T, std::string_view> &&
    requires(T& t, std::string_view name)
    {
      { t.enter(name) } -> std::same_as<Result>;
      { t.leave(name) } -> std::same_as<Result>;
    };

template <class T>
concept TSerializeReader =
    detail::THasSerializeRead<T, uint8_t> &&
    detail::THasSerializeRead<T, uint16_t> &&
    detail::THasSerializeRead<T, uint32_t> &&
    detail::THasSerializeRead<T, uint64_t> &&
    detail::THasSerializeRead<T, int8_t> &&
    detail::THasSerializeRead<T, int16_t> &&
    detail::THasSerializeRead<T, int32_t> &&
    detail::THasSerializeRead<T, int64_t> &&
    detail::THasSerializeRead<T, float> &&
    detail::THasSerializeRead<T, double> &&
    detail::THasSerializeRead<T, std::string> &&
    requires(T& t, std::string_view name)
    {
      { t.enter(name) } -> std::same_as<Result>;
      { t.leave(name) } -> std::same_as<Result>;
    };

/// Specialize this for types that have a <tt>Result serialize(TSerializeWriter auto& writer)</tt> method.
template <class T>
struct IsCustomSerializable : std::false_type {
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
///   return std::tuple {
///     std::make_pair("a", &A::a),
///   };
/// }
/// @endcode
template <class T>
concept HasSerializeMembers = requires { { T::serializeMembers() } -> TMemberTuple; };

/// Checks if a type is either a custom serializable type or has a valid serializeMembers method.
template <class T>
concept TSerializable = IsCustomSerializable<T>::value || HasSerializeMembers<T>;

/// Utility template to get the type from a member pointer
template <class T>
struct MemberType;

template <class C, class T>
struct MemberType<T C::*> {
  using type = std::remove_cvref_t<T>;
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

/// Called for each non-custom serializable field, can be specialized to handle custom serialization needs
auto serialize_field(
    TSerializeWriter auto& writer,
    std::string_view name,
    auto const& value) -> Result {
  return writer.write(name, value);
}

/// Called for each non-custom serializable field, can be specialized to handle custom serialization needs
auto deserialize_field(
    TSerializeReader auto& reader,
    std::string_view name,
    auto& value) -> Result {
  return reader.read(name, value);
}

namespace detail {
template <size_t N, size_t I>
auto deserializeMember(
    TSerializeReader auto& reader,
    detail::TSerializable auto& value,
    auto const& memberPtrTuple) -> Result {
  if constexpr (I < N) {
    auto [name, ptr] = std::get<I>(memberPtrTuple);
    using MemberType = typename detail::MemberType<decltype(ptr)>::type;

    if constexpr (IsCustomSerializable<MemberType>::value || detail::HasSerializeMembers<MemberType>) {
      // If the member is a custom serializable type or supports serializeMembers() then deserialize it recursively
      auto r = reader.enter(name);
      if (!r)
        return r;

      r = deserialize(reader, value.*ptr);
      if (!r)
        return r;

      r = reader.leave(name);
      if (!r)
        return r;
    } else {
      // Otherwise we expect the serialize writer to handle it (as it should be a primitive)
      auto r = deserialize_field(reader, name, value.*ptr);
      if (!r)
        return r;
    }

    return deserializeMember<N, I + 1>(reader, value, memberPtrTuple);
  }
  return {};
}

auto deserializeMembers(
    TSerializeReader auto& reader,
    detail::TSerializable auto& value,
    auto const& memberPtrTuple) {
  // Get the number of name/member function pointer pairs in the tuple
  constexpr auto MemberCount = std::tuple_size_v<std::remove_cvref_t<decltype(memberPtrTuple)>>;
  return deserializeMember<MemberCount, 0>(
      reader,
      value,
      memberPtrTuple);
}

template <size_t N, size_t I>
auto serializeMember(
    TSerializeWriter auto& writer,
    detail::TSerializable auto const& value,
    auto const& memberPtrTuple) -> Result {
  if constexpr (I < N) {
    auto [name, ptr] = std::get<I>(memberPtrTuple);
    using MemberType = typename detail::MemberType<decltype(ptr)>::type;

    if constexpr (IsCustomSerializable<MemberType>::value || detail::HasSerializeMembers<MemberType>) {
      // If the member is a custom serializable type or supports serializeMembers() then serialize it recursively
      auto r = writer.enter(name);
      if (!r)
        return r;

      r = serialize(writer, value.*ptr);
      if (!r)
        return r;

      r = writer.leave(name);
      if (!r)
        return r;
    } else {
      // Otherwise we expect the serialize writer to handle it (as it should be a primitive)
      auto r = serialize_field(writer, name, value.*ptr);
      if (!r)
        return r;
    }

    return serializeMember<N, I + 1>(writer, value, memberPtrTuple);
  }
  return {};
}

auto serializeMembers(
    TSerializeWriter auto& writer,
    detail::TSerializable auto const& value,
    auto const& memberPtrTuple) {
  // Get the number of name/member function pointer pairs in the tuple
  constexpr auto MemberCount = std::tuple_size_v<std::remove_cvref_t<decltype(memberPtrTuple)>>;
  // Start serializing the members from the first one
  return serializeMember<MemberCount, 0>(
      writer,
      value,
      memberPtrTuple);
}
}

[[nodiscard]] auto serialize(
    TSerializeWriter auto& writer,
    detail::TSerializable auto const& value) -> Result {
  if constexpr (IsCustomSerializable<std::remove_cvref_t<decltype(value)>>::value) {
    // If the type is a custom serializable type then as it to serialize its self
    return value.serialize(writer);
  } else {
    // Otherwise serialize each member
    auto members = value.serializeMembers();
    return detail::serializeMembers(writer, value, members);
  }
}

[[nodiscard]] auto deserialize(
    TSerializeReader auto& reader,
    detail::TSerializable auto& value) -> Result {
  if constexpr (IsCustomSerializable<std::remove_cvref_t<decltype(value)>>::value) {
    return value.deserialize(reader);
  } else {
    auto members = value.serializeMembers();
    return detail::deserializeMembers(reader, value, members);
  }
}
}

void test();