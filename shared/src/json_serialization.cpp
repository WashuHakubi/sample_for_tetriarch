/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <memory>
#include <stack>

#include <nlohmann/json.hpp>
#include <shared/serialization.h>

namespace ew {
struct json_writer final : writer {
  json_writer() { current_.push({&root_, false}); }

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

  void to_buffer(std::vector<char>& buffer) const override {
    auto start_pos = buffer.size();
    auto dump = root_.dump(2);
    buffer.resize(buffer.size() + dump.size());
    std::memcpy(buffer.data() + start_pos, dump.data(), dump.size());
  }

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

std::shared_ptr<writer> create_json_writer() {
  return std::make_shared<json_writer>();
}

struct json_reader final : reader {
  json_reader(std::string_view json) : root_(nlohmann::json::parse(json)) { current_.push({&root_, false, SIZE_MAX}); }

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

std::shared_ptr<reader> create_json_reader(std::string_view json) {
  return std::make_shared<json_reader>(json);
}
}