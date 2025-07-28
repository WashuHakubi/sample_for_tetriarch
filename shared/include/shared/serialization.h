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
#include <string_view>
#include <tuple>
#include <type_traits>

namespace ewok::shared {
enum class SerializeError {
  None,
};

/// Base class which is useful for knowing the core set of members to implement
struct ISerializeWriter {
  virtual ~ISerializeWriter() = default;

  virtual auto enter(std::string_view name) -> std::expected<void, SerializeError> = 0;
  virtual auto leave() -> std::expected<void, SerializeError> = 0;

  virtual auto write(std::string_view name, uint8_t value) -> std::expected<void, SerializeError> = 0;
  virtual auto write(std::string_view name, uint16_t value) -> std::expected<void, SerializeError> = 0;
  virtual auto write(std::string_view name, uint32_t value) -> std::expected<void, SerializeError> = 0;
  virtual auto write(std::string_view name, uint64_t value) -> std::expected<void, SerializeError> = 0;
  virtual auto write(std::string_view name, int8_t value) -> std::expected<void, SerializeError> = 0;
  virtual auto write(std::string_view name, int16_t value) -> std::expected<void, SerializeError> = 0;
  virtual auto write(std::string_view name, int32_t value) -> std::expected<void, SerializeError> = 0;
  virtual auto write(std::string_view name, int64_t value) -> std::expected<void, SerializeError> = 0;
  virtual auto write(std::string_view name, float value) -> std::expected<void, SerializeError> = 0;
  virtual auto write(std::string_view name, double value) -> std::expected<void, SerializeError> = 0;
  virtual auto write(std::string_view name, std::string_view value) -> std::expected<void, SerializeError> = 0;
};

namespace detail {
template <class T, class U>
concept THasSerializeWrite = requires(T& t, std::string_view name, U u)
{
  { t.write(name, u) } -> std::same_as<std::expected<void, SerializeError>>;
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
    detail::THasSerializeWrite<T, std::string_view> && requires(T& t, std::string_view name)
    {
      { t.enter(name) } -> std::same_as<std::expected<void, SerializeError>>;
      { t.leave(name) } -> std::same_as<std::expected<void, SerializeError>>;
    };


template <class T>
struct IsCustomSerializable : std::false_type {
};

namespace detail {
template <class T>
struct IsMemberTuple : std::false_type {
};

template <class... Ts>
struct IsMemberTuple<std::tuple<std::pair<char const*, Ts>...>> : std::true_type {
};

template <class T>
concept TMemberTuple = IsMemberTuple<T>::value;

template <class T>
concept HasSerializeMembers = requires { { T::serializeMembers() } -> TMemberTuple; };

template <class T>
concept TSerializable = IsCustomSerializable<T>::value || HasSerializeMembers<T>;

template <class T>
struct MemberType;

template <class C, class T>
struct MemberType<T C::*> {
  using type = std::remove_cvref_t<T>;
};
}

class Serializer {
public:
  static auto serialize(
      TSerializeWriter auto& writer,
      detail::TSerializable auto const& value) -> std::expected<void, SerializeError> {
    if constexpr (IsCustomSerializable<std::remove_cvref_t<decltype(value)>>::value) {
      // If the type is a custom serializable type then as it to serialize its self
      return value.serialize(writer);
    } else {
      // Otherwise serialize each member
      auto members = value.serializeMembers();
      return serializeMembers(
          writer,
          value,
          members);
    }
  }

private:
  static auto serializeMembers(
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

  template <size_t N, size_t I>
  static auto serializeMember(
      TSerializeWriter auto& writer,
      detail::TSerializable auto const& value,
      auto const& memberPtrTuple) -> std::expected<void, SerializeError> {
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
        auto r = writer.write(name, value.*ptr);
        if (!r)
          return r;
      }

      return serializeMember<N, I + 1>(writer, value, memberPtrTuple);
    }
    return {};
  }
};
}

void test();