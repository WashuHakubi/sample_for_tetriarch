/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace wut {
struct IWriter {
  virtual ~IWriter() = default;

  virtual void writeHandle(std::string_view name, int tag) = 0;
  virtual void beginObject(std::string_view name) = 0;
  virtual void endObject() = 0;

  virtual void beginArray(std::string_view name, size_t count) = 0;
  virtual void endArray() = 0;

  virtual void write(std::string_view name, bool v) = 0;

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

  virtual std::string toBuffer() const = 0;
};

std::shared_ptr<IWriter> createJsonWriter();

struct IReader {
  virtual ~IReader() = default;

  virtual bool readHandle(std::string_view name, int& tag) = 0;
  virtual void beginObject(std::string_view name) = 0;
  virtual void endObject() = 0;

  virtual void beginArray(std::string_view name, size_t& count) = 0;
  virtual void endArray() = 0;

  virtual void read(std::string_view name, bool& v) = 0;

  virtual void read(std::string_view name, int8_t& v) = 0;
  virtual void read(std::string_view name, int16_t& v) = 0;
  virtual void read(std::string_view name, int32_t& v) = 0;
  virtual void read(std::string_view name, int64_t& v) = 0;

  virtual void read(std::string_view name, uint8_t& v) = 0;
  virtual void read(std::string_view name, uint16_t& v) = 0;
  virtual void read(std::string_view name, uint32_t& v) = 0;
  virtual void read(std::string_view name, uint64_t& v) = 0;

  virtual void read(std::string_view name, float& v) = 0;
  virtual void read(std::string_view name, double& v) = 0;

  virtual void read(std::string_view name, std::string& v) = 0;
};

std::shared_ptr<IReader> createJsonReader(std::string const& data);

/**
 * Specialize this class, deriving from std::true_type and implementing a static serializeMembers method to return the
 * tuple of member tuples.
 */
template <class T>
struct SerializeMembers : std::false_type {
  // static auto serializeMembers();
};

/**
 * Creates a shared pointer to T using the default constructor. Specialize this for types that do not have a default
 * constructor.
 */
template <class T>
struct SerializeFactory {
  static std::shared_ptr<T> create() { return std::make_shared<T>(); }
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

  static void reserve(std::vector<T, A>& v, size_t count) { v.reserve(count); }

  template <class U>
  static void push_back(std::vector<T, A>& v, size_t i, U&& val) {
    v.push_back(std::forward<T>(val));
  }
};

template <class T, size_t N>
struct isArray<std::array<T, N>> : std::true_type {
  static constexpr bool canResize = false;

  static void reserve(std::array<T, N>& v, size_t count) {}

  template <class U>
  static void push_back(std::array<T, N>& v, size_t i, U&& val) {
    v[i] = std::forward<T>(val);
  }
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

template <class T>
concept IsSerializableClass =
    std::is_class_v<T> && (detail::hasSerializeMembersMethod<T> || SerializeMembers<T>::value);

// Specialize this to provide custom serialization of types.
template <class C>
void writeSpecialized(IWriter& writer, std::string_view name, C const& obj);

// Specialize this to provide custom deserialization of types.
template <class C>
void readSpecialized(IReader& reader, std::string_view name, C& obj);

namespace detail {
template <class C>
void writeObjectInternal(
    IWriter& writer,
    std::string_view name,
    C const& obj,
    std::unordered_map<std::shared_ptr<void>, int>& tagMap) {
  if constexpr (std::is_fundamental_v<C> || std::is_convertible_v<C, std::string_view>) {
    // If we have a fundamental type (int, char, float, etc.), or we have something we can convert to a string_view
    // (std::string) then write it out.
    writer.write(name, obj);
  } else if constexpr (detail::isSharedPtr<C>::value) {
    // Check if we have already written out an object matching this shared pointer, if so write out that tag.
    // Otherwise remember the tag and write out the object.
    if (tagMap.contains(obj)) {
      writer.writeHandle(name, tagMap[obj]);
    } else {
      tagMap.emplace(obj, static_cast<int>(tagMap.size()));
      writeObjectInternal(writer, name, *obj, tagMap);
    }
  } else if constexpr (detail::isArray<C>::value) {
    // If we have an array type then write out each element of the array.
    writer.beginArray(name, obj.size());
    for (auto const& val : obj) {
      writeObjectInternal(writer, "", val, tagMap);
    }
    writer.endArray();
  } else if constexpr (IsSerializableClass<C>) {
    // If we have some class type then write out each member of the class.
    writer.beginObject(name);
    auto members = detail::getSerializeMembers<C>();
    detail::forEachTupleItem(members, [&writer, &obj, &tagMap](auto const& memberTuple) {
      auto& val = obj.*std::get<1>(memberTuple);
      writeObjectInternal(writer, std::get<0>(memberTuple), val, tagMap);
    });
    writer.endObject();
  } else {
    writeSpecialized(writer, name, obj);
  }
}
} // namespace detail

template <class C>
void writeObject(IWriter& writer, std::string_view name, C const& obj) {
  auto tagMap = std::unordered_map<std::shared_ptr<void>, int>{{nullptr, -1}};
  detail::writeObjectInternal(writer, name, obj, tagMap);
}

namespace detail {
template <class C>
void readObjectInternal(
    IReader& reader,
    std::string_view name,
    C& obj,
    std::unordered_map<int, std::shared_ptr<void>>& tagMap) {
  if constexpr (std::is_fundamental_v<C> || std::is_same_v<C, std::string>) {
    // If we have a fundamental type (int, char, float, etc.), or we have a std::string then read it.
    reader.read(name, obj);
  } else if constexpr (isSharedPtr<C>::value) {
    // If we have a pointer then it might be a tag to an existing pointer, check that. If not read in the object.
    int tag;
    if (reader.readHandle(name, tag)) {
      if (tag == -1) {
        // Object was not found, and neither was a tag, must be a nullptr.
        obj = nullptr;
        return;
      }

      // Found an existing tag, fetch shared pointer and set it.
      auto it = tagMap.find(tag);
      assert(it != tagMap.end());
      obj = std::static_pointer_cast<typename C::element_type>(it->second);
    } else {
      // Not encountered this object before, create it and map to that tag.
      obj = SerializeFactory<typename C::element_type>::create();
      tagMap.emplace(tag, obj);
      readObjectInternal(reader, name, *obj, tagMap);
    }
  } else if constexpr (isArray<C>::value) {
    // If we have an array type then read each element of the array.
    size_t count;
    reader.beginArray(name, count);
    detail::isArray<C>::reserve(obj, count);

    for (size_t i = 0; i < count; ++i) {
      typename C::value_type val;
      readObjectInternal(reader, "", val, tagMap);
      detail::isArray<C>::push_back(obj, i, std::move(val));
    }
    reader.endArray();
  } else if constexpr (IsSerializableClass<C>) {
    // If we have some class type then write out each member of the class.
    reader.beginObject(name);
    auto members = detail::getSerializeMembers<C>();
    forEachTupleItem(members, [&reader, &obj, &tagMap](auto const& memberTuple) {
      auto& val = obj.*std::get<1>(memberTuple);
      readObjectInternal(reader, std::get<0>(memberTuple), val, tagMap);
    });
    reader.endObject();
  } else {
    readSpecialized(reader, name, obj);
  }
}
} // namespace detail

template <class C>
void readObject(IReader& reader, std::string_view name, C& obj) {
  std::unordered_map<int, std::shared_ptr<void>> tagMap;
  detail::readObjectInternal(reader, name, obj, tagMap);
}
} // namespace wut
