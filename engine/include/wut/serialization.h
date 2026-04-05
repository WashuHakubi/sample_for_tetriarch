/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <glm/ext.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

  virtual void write(std::string_view name, std::nullptr_t) = 0;

  virtual std::string toBuffer() const = 0;
};

std::shared_ptr<IWriter> createJsonWriter();

struct IReader {
  virtual ~IReader() = default;

  virtual bool has(std::string_view name) = 0;

  virtual bool readHandle(std::string_view name, int& tag) = 0;

  virtual void enumerate(std::function<void(IReader& reader, std::string_view name)> const& fn) = 0;

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

  virtual bool read(std::string_view name, std::nullptr_t) = 0;
};

std::shared_ptr<IReader> createJsonReader(std::string_view data);

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

template <class T>
struct DeserializeObserver {
  static void onDeserialized(T& obj) {}
};

// Specialize this to provide custom deserialization of types.
template <class T>
void readSpecialized(
    IReader& reader,
    std::string_view name,
    T& obj,
    std::unordered_map<int, std::shared_ptr<void>>& tagMap);

template <class T>
struct DefaultValue {
  T value;
};

namespace detail {
template <class T>
struct isSharedPtr : std::false_type {};

template <class T>
struct isSharedPtr<std::shared_ptr<T>> : std::true_type {};

template <class T>
struct isArray : std::false_type {};

// Specialization of isArray for std::vector<T, A>
template <class T, class A>
struct isArray<std::vector<T, A>> : std::true_type {
  static constexpr bool canResize = true;

  static void reserve(std::vector<T, A>& v, size_t count) { v.reserve(count); }

  template <class U>
  static void push_back(std::vector<T, A>& v, size_t i, U&& val) {
    v.push_back(std::forward<T>(val));
  }
};

// Specialization of isArray for std::array<T, N>
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

/**
 * True if a type has a static serializeMembers() method, or if SerializeMembers<T> has been specialized.
 */
template <class T>
concept IsSerializableClass =
    std::is_class_v<T> && (detail::hasSerializeMembersMethod<T> || SerializeMembers<T>::value);

template <IsSerializableClass T>
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

/**
 * Checks if a tuple contains a type.
 */
template <typename T, typename Tuple>
struct HasType;

template <typename T>
struct HasType<T, std::tuple<>> : std::false_type {};

template <typename T, typename U, typename... Ts>
struct HasType<T, std::tuple<U, Ts...>> : HasType<T, std::tuple<Ts...>> {};

template <typename T, typename... Ts>
struct HasType<T, std::tuple<T, Ts...>> : std::true_type {};

/**
 * Appends members to a tuple of members.
 */
template <class Tuple, class... MemberTuples>
auto catMembers(Tuple&& tuple, MemberTuples&&... memberTuples) {
  return std::tuple_cat(std::forward<Tuple>(tuple), std::make_tuple(std::forward<MemberTuples>(memberTuples)...));
}

template <class T>
std::string nameOf() {
  std::string_view sv;
#if defined(__clang__)
  // clang-format off
  // std::string wut::detail::nameOf() [T = int]
  // clang-format on
  sv = __PRETTY_FUNCTION__;
  auto tf = 39;
  auto te = 1;
#elif defined(__GNUC__)
  // clang-format off
  // std::string wut::detail::nameOf() [with T = int; std::string = std::__cxx11::basic_string<char>]
  // clang-format on
  sv = __PRETTY_FUNCTION__;
  auto tf = 44;
  auto te = 49;
#elif defined(_MSC_VER)
  // clang-format off
  // class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > __cdecl wut::detail::nameOf<int>(void)
  // clang-format on
  sv = __FUNCSIG__;
  auto tf = 116;
  auto te = 7;
#else
#error Unknown compiler
#endif

  sv.remove_prefix(tf);
  sv.remove_suffix(te);

#if defined(_MSC_VER)
  if (sv.starts_with('c')) {
    sv.remove_prefix(6);
  } else if (sv.starts_with('s')) {
    sv.remove_prefix(7);
  } else if (sv.starts_with('e')) {
    sv.remove_prefix(5);
  }
#endif

  return std::string(sv.begin(), sv.end());
}
} // namespace detail

using WriteTags = std::unordered_map<std::shared_ptr<void>, int>;
using ReadTags = std::unordered_map<int, std::shared_ptr<void>>;

/**
 * Reader declaration. This can be overriden to perform specific deserialization of types.
 */
template <class T>
void readObject(IReader& reader, std::string_view name, T& obj, ReadTags& tags);

/**
 * Utility method to make an reading object a bit easier.
 */
template <class T>
void read(IReader& reader, T& obj) {
  ReadTags tags;
  return readObject(reader, "", obj, tags);
}

/**
 * Supports reading fundmanetal and string types.
 */
template <class T>
  requires((std::is_fundamental_v<T> || std::is_same_v<std::string, T>))
void readObject(IReader& reader, std::string_view name, T& obj, ReadTags& tags) {
  return reader.read(name, obj);
}

/**
 * Supports reading shared pointer types. If the pointer is null it will initialize obj to a null. If the pointer has
 * already been read then it will read the tag pointing to the object and initialize the pointer to that value.
 */
template <class T>
  requires(detail::isSharedPtr<T>::value)
void readObject(IReader& reader, std::string_view name, T& obj, ReadTags& tags) {
  if (reader.read(name, nullptr)) {
    obj = nullptr;
    return;
  }

  int tag;
  if (reader.readHandle(name, tag)) {
    auto it = tags.find(tag);
    assert(it != tags.end());
    obj = std::static_pointer_cast<typename T::element_type>(it->second);
    return;
  }

  obj = SerializeFactory<typename T::element_type>::create();
  tags.emplace(tags.size(), obj);
  readObject(reader, name, *obj, tags);
}

/**
 * Supports reading types that have an isArray specialization.
 */
template <class T>
  requires(detail::isArray<T>::value)
void readObject(IReader& reader, std::string_view name, T& obj, ReadTags& tags) {
  size_t count;
  reader.beginArray(name, count);
  detail::isArray<T>::reserve(obj, count);

  for (size_t i = 0; i < count; ++i) {
    typename T::value_type val;
    readObject(reader, "", val, tags);
    detail::isArray<T>::push_back(obj, i, std::move(val));
  }
  reader.endArray();
}

/**
 * Supports reading types that have a static serializeMembers() method, or SerializeMembers<T> specialization
 */
template <class T>
  requires(detail::IsSerializableClass<T>)
void readObject(IReader& reader, std::string_view name, T& obj, ReadTags& tags) {
  reader.beginObject(name);
  auto members = detail::getSerializeMembers<T>();
  detail::forEachTupleItem(members, [&reader, &obj, &tags](auto const& memberTuple) {
    auto& val = obj.*std::get<1>(memberTuple);
    using ValType = std::decay_t<decltype(val)>;

    if constexpr (detail::HasType<DefaultValue<ValType>, std::decay_t<decltype(memberTuple)>>::value) {
      if (!reader.has(std::get<0>(memberTuple))) {
        val = std::get<DefaultValue<ValType>>(memberTuple).value;
        return;
      }
    }
    readObject(reader, std::get<0>(memberTuple), val, tags);
  });
  reader.endObject();

  DeserializeObserver<T>::onDeserialized(obj);
}

/**
 * Supports reading an optional value. If name exists in the reader then the value will be read. Otherwise the optional
 * is left at the default value it was constructed with.
 */
template <class T>
void readObject(IReader& reader, std::string_view name, std::optional<T>& obj, ReadTags& tags) {
  if (reader.has(name)) {
    obj = T{};
    readObject(reader, name, obj.value(), tags);
  }
}

template <class K, class V, class H, class P, class A>
void readObject(IReader& reader, std::string_view name, std::unordered_map<K, V, H, P, A>& obj, ReadTags& tags) {
  reader.beginObject(name);
  reader.enumerate([&obj, &tags](IReader& reader, std::string_view key) {
    readObject(reader, (K)key, obj[(K)key], tags);
  });
  reader.endObject();
}

/**
 * Writer declaration. This can be overriden to specialize serialization of types.
 */
template <class T>
void writeObject(IWriter& writer, std::string_view name, T const& obj, WriteTags& tags);

/**
 * Utility method to make writing an object a bit easier.
 */
template <class T>
void write(IWriter& writer, T const& obj) {
  WriteTags tags;
  return writeObject(writer, "", obj, tags);
}

/**
 * Supports writing out fundmanetal and string types that are convertible to a string_view.
 */
template <class T>
  requires((std::is_fundamental_v<T> || std::is_convertible_v<T, std::string_view>))
void writeObject(IWriter& writer, std::string_view name, T const& obj, WriteTags& tags) {
  return writer.write(name, obj);
}

/**
 * Supports writing shared pointer types. If the pointer is null it will write a null. If the pointer has already been
 * written then a tag pointing to the written object is written instead.
 */
template <class T>
  requires(detail::isSharedPtr<T>::value)
void writeObject(IWriter& writer, std::string_view name, T const& obj, WriteTags& tags) {
  if (obj == nullptr) {
    writer.write(name, nullptr);
    return;
  }

  if (auto it = tags.find(obj); it != tags.end()) {
    writer.writeHandle(name, it->second);
    return;
  }

  tags.emplace(obj, tags.size());
  return writeObject(writer, name, *obj, tags);
}

/**
 * Supports writing types that have an isArray specialization.
 */
template <class T>
  requires(detail::isArray<T>::value)
void writeObject(IWriter& writer, std::string_view name, T const& obj, WriteTags& tags) {
  writer.beginArray(name, obj.size());
  for (auto&& elem : obj) {
    writeObject(writer, "", elem, tags);
  }
  writer.endArray();
}

/**
 * Supports writing types that have a static serializeMembers() method, or SerializeMembers<T> specialization.
 */
template <class T>
  requires(detail::IsSerializableClass<T>)
void writeObject(IWriter& writer, std::string_view name, T const& obj, WriteTags& tags) {
  writer.beginObject(name);
  auto members = detail::getSerializeMembers<T>();
  detail::forEachTupleItem(members, [&writer, &obj, &tags](auto const& memberTuple) {
    auto& val = obj.*std::get<1>(memberTuple);
    writeObject(writer, std::get<0>(memberTuple), val, tags);
  });
  writer.endObject();
}

/**
 * Supports writing optional fields. The field will not be written if it does not have a value.
 */
template <class T>
void writeObject(IWriter& writer, std::string_view name, std::optional<T> const& obj, WriteTags& tags) {
  if (obj.has_value()) {
    writeObject(writer, name, obj.value(), tags);
  }
}

template <class K, class V, class H, class P, class A>
void writeObject(
    IWriter& writer,
    std::string_view name,
    const std::unordered_map<K, V, H, P, A>& obj,
    WriteTags& tags) {
  writer.beginObject(name);
  for (auto&& [k, v] : obj) {
    writeObject(writer, k, v, tags);
  }
  writer.endObject();
}

void readObject(IReader& reader, std::string_view name, glm::vec2& obj, ReadTags& tags);

void writeObject(IWriter& writer, std::string_view name, const glm::vec2& obj, WriteTags& tags);

void readObject(IReader& reader, std::string_view name, glm::vec3& obj, ReadTags& tags);

void writeObject(IWriter& writer, std::string_view name, const glm::vec3& obj, WriteTags& tags);

void readObject(IReader& reader, std::string_view name, glm::vec4& obj, ReadTags& tags);

void writeObject(IWriter& writer, std::string_view name, const glm::vec4& obj, WriteTags& tags);

void readObject(IReader& reader, std::string_view name, glm::quat& obj, ReadTags& tags);

void writeObject(IWriter& writer, std::string_view name, const glm::quat& obj, WriteTags& tags);

void readObject(IReader& reader, std::string_view name, glm::mat4x4& obj, ReadTags& tags);

void writeObject(IWriter& writer, std::string_view name, const glm::mat4x4& obj, WriteTags& tags);
} // namespace wut

#define SERIALIZE_MEMBER(N, ...) std::make_tuple(#N, &Type::N, ##__VA_ARGS__)
