/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace wut {
struct IWriter {
  virtual ~IWriter() = default;

  virtual bool beginObject(std::string_view name, void* tag = nullptr) = 0;
  virtual void endObject() = 0;

  virtual void beginArray(std::string_view name, size_t count) = 0;
  virtual void endArray() = 0;

  virtual void write(std::string_view name, bool v) = 0;

  virtual void write(std::string_view name, char v) = 0;

  virtual void write(std::string_view name, int8_t v) = 0;
  virtual void write(std::string_view name, int16_t v) = 0;
  virtual void write(std::string_view name, int32_t v) = 0;
  virtual void write(std::string_view name, int64_t v) = 0;

  virtual void write(std::string_view name, uint8_t v) = 0;
  virtual void write(std::string_view name, uint16_t v) = 0;
  virtual void write(std::string_view name, uint32_t v) = 0;
  virtual void write(std::string_view name, uint64_t v) = 0;

  virtual void write(std::string_view name, float v) = 0;
  virtual void write(std::string_view name, double v) = 0;

  virtual void write(std::string_view name, std::string_view v) = 0;
};

/**
 * Specialize this class, deriving from std::true_type and implementing a static serializeMembers method to return the
 * tuple of member tuples.
 */
template <class T>
struct SerializeMembers : std::false_type {
  // static auto serializeMembers();
};

namespace detail {
template <class T>
struct isSharedPtr : std::false_type {};

template <class T>
struct isSharedPtr<std::shared_ptr<T>> : std::true_type {};

template <class T>
struct isArray : std::false_type {};

template <class T, class A>
struct isArray<std::vector<T, A>> : std::true_type {
  static constexpr bool canResize = true;
};

template <class T, size_t N>
struct isArray<std::array<T, N>> : std::true_type {
  static constexpr bool canResize = false;
};

template <class T>
concept hasSerializeMembersMethod = requires {
  { T::serializeMembers() };
};

template <class T>
auto getSerializeMembers() {
  if constexpr (hasSerializeMembersMethod<T>) {
    return T::serializeMembers();
  } else if constexpr (SerializeMembers<T>::value) {
    return SerializeMembers<T>::serializeMembers();
  } else {
    static_assert(false, "Must have either a serializeMembers method or SerializeMembers specialization");
  }
}

// Iterates through each element in a tuple calling fn() on them.
template <class T, class Fn, int I = 0>
auto forEachTupleItem(T const& t, Fn&& fn) {
  using Tuples = std::decay_t<T>;

  if constexpr (I < std::tuple_size_v<Tuples>) {
    fn(std::get<I>(t));
    forEachTupleItem<T, Fn, I + 1>(t, std::forward<Fn>(fn));
  }
}
} // namespace detail

template <class C>
void writeObject(IWriter& writer, std::string_view name, C const& obj) {
  if constexpr (std::is_fundamental_v<C> || std::is_convertible_v<C, std::string_view>) {
    // If we have a fundamental type (int, char, float, etc.), or we have something we can convert to a string_view
    // (std::string) then write it out.
    writer.write(name, obj);
  } else if constexpr (detail::isSharedPtr<C>::value) {
    // If we have a pointer to an object then we attempt to write it out, if beginObject returns false then we can
    // assume we have already written a copy of this object out, and that a tag has been written out instead.
    // If true is returned then we should proceed to write out the object.
    // TODO: Make this handled fundamental types like std::shared_ptr<int>?
    auto ptr = obj.get();
    if (!ptr) {
      // If obj is a nullptr then don't write anything.
      return;
    }
    if (writer.beginObject(name, ptr)) {
      auto members = detail::getSerializeMembers<typename C::element_type>();
      detail::forEachTupleItem(members, [&writer, ptr](auto const& memberTuple) {
        auto const& val = ptr->*std::get<1>(memberTuple);
        writeObject(writer, std::get<0>(memberTuple), val);
      });
      writer.endObject();
    }
  } else if constexpr (detail::isArray<C>::value) {
    // If we have an array type then write out each element of the array.
    writer.beginArray(name, obj.size());
    for (auto const& val : obj) {
      writeObject(writer, "", val);
    }
    writer.endArray();
  } else if constexpr (std::is_class_v<C>) {
    // If we have some class type then write out each member of the class.
    writer.beginObject(name);
    auto members = detail::getSerializeMembers<C>();
    detail::forEachTupleItem(members, [&writer, &obj](auto const& memberTuple) {
      auto const& val = obj.*std::get<1>(memberTuple);
      writeObject(writer, std::get<0>(memberTuple), val);
    });
    writer.endObject();
  } else {
    // Don't know what this is, so we should add code to make it work.
    static_assert(false, "Failed to write object");
  }
}
} // namespace wut
