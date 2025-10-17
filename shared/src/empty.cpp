/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <stack>

#include <shared/serialization.h>

namespace ew {
struct vec3 {
  float x, y, z;

  bool operator==(const vec3&) const = default;
  bool operator!=(const vec3&) const = default;
};

// Example of a custom serializer for a type, writes vec3's as arrays of floats
void serialize(writer& writer, std::string_view name, vec3 const& v) {
  writer.begin_object(name);
  writer.write("x", v.x);
  writer.write("y", v.y);
  writer.write("z", v.z);
  writer.end_object();
}

void serialize(writer& writer, std::string_view name, vec3 const& v, attrs::compress_tag) {
  writer.begin_array(name, 3);
  writer.write(name, v.x);
  writer.write(name, v.y);
  writer.write(name, v.z);
  writer.end_array();
}

void deserialize(reader& reader, std::string_view name, vec3& v) {
  reader.begin_object(name);
  reader.read("x", v.x);
  reader.read("y", v.y);
  reader.read("z", v.z);
  reader.end_object();
}

void deserialize(reader& reader, std::string_view name, vec3& v, attrs::compress_tag) {
  size_t count;
  reader.begin_array(name, count);
  assert(count == 3);
  reader.read(name, v.x);
  reader.read(name, v.y);
  reader.read(name, v.z);
  reader.end_array();
}

struct sample {
  unsigned a;
  vec3 v;
  std::array<vec3, 4> va;
  std::vector<bool> bs;

  bool operator==(const sample&) const = default;
  bool operator!=(const sample&) const = default;
};
} // namespace ew

// EW_REFLECT(ew::vec3) {
//   return std::make_tuple(
//       std::make_tuple("x", &vec3::x),
//       std::make_tuple("y", &vec3::y),
//       std::make_tuple("z", &vec3::z));
// }

EW_REFLECT(ew::sample) {
  return std::make_tuple(
      std::make_tuple("a", &sample::a, attrs::compress, attrs::allowed_range<unsigned>{0, 42}),
      std::make_tuple("v", &sample::v),
      std::make_tuple("va", &sample::va, attrs::compress),
      std::make_tuple("bs", &sample::bs));
}

namespace ew {
template <class T>
void validate(attrs::errors& err, std::string const& path, T const& value) {
  if constexpr (ew::reflect<T>::value) {
    ew::apply(
        [&value, &err, &path](auto const& member) {
          auto p = path + "." + std::get<0>(member);
          auto member_ptr = std::get<1>(member);
          using member_type = std::decay_t<decltype(value.*member_ptr)>;

          // Check each member of the tuple, if there is a valid validate() member then call it
          ew::apply(
              [&value, &err, &p, &member_ptr]<typename Tuple>(Tuple const& attr) {
                if constexpr (attrs::validatable<std::decay_t<Tuple>, member_type>) {
                  attr.validate(err, p, value.*member_ptr);
                }
              },
              member);

          // Recurse into reflected member types, this allows for custom validation to be an overload of the validate
          // function provided the top level type being validated is reflectable.
          validate(err, p, value.*member_ptr);
        },
        reflect<T>::members());
  }
}

template <class T, size_t N>
void validate(attrs::errors& err, std::string const& path, std::array<T, N> const& value) {
  for (size_t i = 0; i < value.size(); ++i) {
    validate(err, std::format("{}[{}]", path, i), value[i]);
  }
}

template <class T, class Alloc>
void validate(attrs::errors& err, std::string const& path, std::vector<T, Alloc> const& value) {
  for (size_t i = 0; i < value.size(); ++i) {
    validate(err, std::format("{}[{}]", path, i), value[i]);
  }
}

void validate(attrs::errors& err, std::string const& path, vec3 const& value) {
  if (value.x >= 5) {
    err.append(path + ".x", "was greater than 5");
  }
}

void f() {
  sample s{
      44,
      {1, 2, 3},
      {
          vec3{1, 2, 3},
          vec3{3, 4, 5},
          vec3{5, 6, 7},
          vec3{7, 8, 9},
      },
      {true, false, true, false},
  };

  attrs::errors err;
  validate(err, "s", s);
  if (!err.success()) {
    for (auto&& e : err.items()) {
      std::cout << e << std::endl;
    }
  }

  auto w = create_json_writer();
  serialize(*w, s);

  std::vector<char> buffer;
  w->to_buffer(buffer);
  std::cout << std::string_view{buffer} << std::endl;

  sample s2;
  auto r = create_json_reader(std::string_view{buffer});
  deserialize(*r, s2);

  assert(s == s2);

  buffer.clear();
  auto wb = create_binary_writer(buffer);
  uint64_t test = 0x7fff;
  wb->write_compressed("", test);
  wb->write_compressed("", UINT64_MAX);

  auto rb = create_binary_reader(buffer);
  uint64_t test2{};
  rb->read_compressed("", test2);
  assert(test == test2);

  rb->read_compressed("", test2);
  assert(test2 == UINT64_MAX);
}
} // namespace ew
