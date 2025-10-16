/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <stack>

#include <nlohmann/json.hpp>
#include <shared/serialization.h>

namespace ew {
struct sample_writer final : writer {
  sample_writer() { current_.push({&root_, false}); }

  void write(std::string_view name, bool value) override { write_impl(name, value); }

  void write(std::string_view name, uint8_t value) override { write_impl(name, value); }
  void write(std::string_view name, uint16_t value) override { write_impl(name, value); }
  void write(std::string_view name, uint32_t value) override { write_impl(name, value); }
  void write(std::string_view name, uint64_t value) override { write_impl(name, value); }

  void write(std::string_view name, int8_t value) override { write_impl(name, value); }
  void write(std::string_view name, int16_t value) override { write_impl(name, value); }
  void write(std::string_view name, int32_t value) override { write_impl(name, value); }
  void write(std::string_view name, int64_t value) override { write_impl(name, value); }

  void write_compressed(std::string_view name, uint16_t value) override { write_impl(name, value); }
  void write_compressed(std::string_view name, uint32_t value) override { write_impl(name, value); }
  void write_compressed(std::string_view name, uint64_t value) override { write_impl(name, value); }

  void write(std::string_view name, float value) override { write_impl(name, value); }
  void write(std::string_view name, double value) override { write_impl(name, value); }

  void write(std::string_view name, std::string_view value) override { write_impl(name, value); }

  void begin_object(std::string_view name) override {
    auto& [curr, is_array] = current_.top();
    if (is_array) {
      curr->push_back(nlohmann::json::object());
      current_.push({&curr->back(), false});
    } else {
      auto& next_curr = (*curr)[name] = nlohmann::json::object();
      current_.push({&next_curr, false});
    }
  }

  void end_object() override { current_.pop(); }

  void begin_array(std::string_view name, size_t count) override {
    auto& [curr, is_array] = current_.top();
    if (is_array) {
      curr->push_back(nlohmann::json::array());
      current_.push({&curr->back(), true});
    } else {
      auto& next_curr = (*curr)[name] = nlohmann::json::array();
      current_.push({&next_curr, true});
    }
  }

  void end_array() override { current_.pop(); }

  std::string to_string() const { return root_.dump(2); }

 private:
  template <class T>
  void write_impl(std::string_view name, T const& v) {
    auto& [curr, is_array] = current_.top();
    if (is_array) {
      curr->push_back(v);
    } else {
      (*curr)[name] = v;
    }
  }

  nlohmann::json root_;
  std::stack<std::pair<nlohmann::json*, bool>> current_;
};

struct sample_reader final : reader {
  sample_reader(std::string json) : root_(nlohmann::json::parse(json)) { current_.push({&root_, false, SIZE_MAX}); }

  void read(std::string_view name, bool& value) override { read_impl(name, value); }

  void read(std::string_view name, uint8_t& value) override { read_impl(name, value); }
  void read(std::string_view name, uint16_t& value) override { read_impl(name, value); }
  void read(std::string_view name, uint32_t& value) override { read_impl(name, value); }
  void read(std::string_view name, uint64_t& value) override { read_impl(name, value); }

  void read(std::string_view name, int8_t& value) override { read_impl(name, value); }
  void read(std::string_view name, int16_t& value) override { read_impl(name, value); }
  void read(std::string_view name, int32_t& value) override { read_impl(name, value); }
  void read(std::string_view name, int64_t& value) override { read_impl(name, value); }

  void read_compressed(std::string_view name, uint16_t& value) override { read_impl(name, value); }
  void read_compressed(std::string_view name, uint32_t& value) override { read_impl(name, value); }
  void read_compressed(std::string_view name, uint64_t& value) override { read_impl(name, value); }

  void read(std::string_view name, float& value) override { read_impl(name, value); }
  void read(std::string_view name, double& value) override { read_impl(name, value); }

  void read(std::string_view name, std::string& value) override { read_impl(name, value); }

  void begin_object(std::string_view name) override {
    auto& [curr, is_array, index] = current_.top();
    if (is_array) {
      assert(index < curr->size());
      auto& next_curr = curr->at(index);
      assert(next_curr.is_object());
      current_.push({&next_curr, false, SIZE_MAX});
      ++index;
    } else {
      auto& next_curr = (*curr)[name];
      assert(next_curr.is_object());
      current_.push({&next_curr, false, SIZE_MAX});
    }
  }

  void end_object() override { current_.pop(); }

  void begin_array(std::string_view name, size_t& count) override {
    auto& [curr, is_array, index] = current_.top();
    if (is_array) {
      assert(index < curr->size());
      auto& next_curr = curr->at(index);
      assert(next_curr.is_array());
      count = next_curr.size();
      current_.push({&next_curr, true, 0});
      ++index;
    } else {
      auto& next_curr = (*curr)[name];
      assert(next_curr.is_array());
      count = next_curr.size();
      current_.push({&next_curr, true, 0});
    }
  }

  void end_array() override { current_.pop(); }

 private:
  template <class T>
  void read_impl(std::string_view name, T& value) {
    auto& [curr, is_array, index] = current_.top();
    if (is_array) {
      value = curr->at(index).get<T>();
      ++index;
    } else {
      value = (*curr)[name];
    }
  }

  nlohmann::json root_;
  std::stack<std::tuple<nlohmann::json*, bool, size_t>> current_;
};

struct vec3 {
  float x, y, z;

  bool operator==(const vec3&) const = default;
  bool operator!=(const vec3&) const = default;
};

//
// EW_REFLECT(ew::vec3) {
//   return std::make_tuple(
//       std::make_tuple("x", &vec3::x),
//       std::make_tuple("y", &vec3::y),
//       std::make_tuple("z", &vec3::z));
// }

// Example of a custom serializer for a type, writes vec3's as arrays of floats
void serialize(writer& writer, std::string_view name, vec3 const& v) {
  writer.begin_array(name, 3);
  writer.write(name, v.x);
  writer.write(name, v.y);
  writer.write(name, v.z);
  writer.end_array();
}

void deserialize(reader& reader, std::string_view name, vec3& value) {
  size_t count;
  reader.begin_array(name, count);
  assert(count == 3);
  reader.read(name, value.x);
  reader.read(name, value.y);
  reader.read(name, value.z);
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

EW_REFLECT(ew::sample) {
  return std::make_tuple(
      std::make_tuple("a", &sample::a, attrs::compress, attrs::allowed_range<unsigned>{0, 42}),
      std::make_tuple("v", &sample::v),
      std::make_tuple("va", &sample::va),
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

  sample_writer w;
  serialize(w, "", s);

  std::cout << w.to_string() << std::endl;

  sample s2;
  sample_reader r(w.to_string());
  deserialize(r, "", s2);

  assert(s == s2);
}
} // namespace ew
