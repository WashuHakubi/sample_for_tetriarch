/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/serialization.h>

#include <stack>
#include <nlohmann/json.hpp>

namespace wut {
struct JsonWriter : IWriter {
  JsonWriter() {}

  bool beginObject(std::string_view name, void* tag) override {
    assert(root_.is_null() || root_.is_object());
    if (root_.is_null()) {
      root_ = nlohmann::json::object();
      nested_.emplace(&root_, true);
      if (name.empty()) {
        return true;
      }
    }

    auto& [cur, isObj] = nested_.top();
    // If we do not have a tag, or we have never seen this tag, map it to an ID
    if (tag == nullptr || tagToId_.emplace(tag, lastId_).second) {
      if (tag != nullptr) {
        ++lastId_;
      }

      // Since we've never seen this object, create an object and return true.
      if (cur->is_object()) {
        auto& jo = (*cur)[name] = nlohmann::json::object();
        nested_.emplace(&jo, true);
      } else {
        cur->push_back(nlohmann::json::object());
        nested_.emplace(&cur->back(), true);
      }
      return true;
    } else {
      // We've seen this tag, so just use the tag.
      if (isObj) {
        (*cur)[name] = "&" + std::to_string(tagToId_[tag]);
      } else {
        cur->push_back("&" + std::to_string(tagToId_[tag]));
      }
      return false;
    }
  }

  void endObject() override {
    assert(!nested_.empty());
    nested_.pop();
  }

  void beginArray(std::string_view name, size_t count) override {
    assert(root_.is_null() || root_.is_object());
    if (root_.is_null()) {
      root_ = nlohmann::json::object();
      nested_.emplace(&root_, true);
    }

    auto& [cur, isObj] = nested_.top();

    // Nested arrays are not supported
    assert(isObj);

    auto& jo = (*cur)[name] = nlohmann::json::array();
    nested_.emplace(&jo, false);
  }

  void endArray() override { endObject(); }

  void write(std::string_view name, bool v) override { writeEntry(name, v); }

  void write(std::string_view name, char v) override { writeEntry(name, v); }

  void write(std::string_view name, int8_t v) override { writeEntry(name, v); }

  void write(std::string_view name, int16_t v) override { writeEntry(name, v); }

  void write(std::string_view name, int32_t v) override { writeEntry(name, v); }

  void write(std::string_view name, int64_t v) override { writeEntry(name, v); }

  void write(std::string_view name, uint8_t v) override { writeEntry(name, v); }

  void write(std::string_view name, uint16_t v) override { writeEntry(name, v); }

  void write(std::string_view name, uint32_t v) override { writeEntry(name, v); }

  void write(std::string_view name, uint64_t v) override { writeEntry(name, v); }

  void write(std::string_view name, float v) override { writeEntry(name, v); }

  void write(std::string_view name, double v) override { writeEntry(name, v); }

  void write(std::string_view name, std::string_view v) override { writeEntry(name, v); }

  void toBuffer(std::vector<char>& out) const override {
    auto s = root_.dump(2);
    out.insert(out.end(), s.begin(), s.end());
  }

 private:
  template <class T>
  void writeEntry(std::string_view name, T&& v) {
    auto& [cur, isObj] = nested_.top();
    assert(isObj || name.empty());

    if (isObj) {
      (*cur)[name] = v;
    } else {
      cur->push_back(v);
    }
  }

  nlohmann::json root_{nlohmann::json(nullptr)};
  std::stack<std::tuple<nlohmann::json*, bool>> nested_;
  int lastId_{0};
  std::unordered_map<void*, int> tagToId_;
};

std::shared_ptr<IWriter> createJsonWriter() {
  return std::make_shared<JsonWriter>();
}

struct JsonReader : IReader {
  JsonReader(std::string const& data) { root_ = nlohmann::json::parse(data); }

  bool beginObject(std::string_view name, int* tag = nullptr) override {
    if (nested_.empty()) {
      assert(root_.is_object());
      nested_.emplace(&root_, true, SIZE_MAX);
      if (tag) {
        *tag = -1;
      }
      return true;
    }

    auto& [cur, isObj, idx] = nested_.top();
    nlohmann::json::value_type* jo;
    if (isObj) {
      jo = &cur->at(name);
    } else {
      jo = &cur->at(idx++);
    }

    if (jo->is_object()) {
      nested_.emplace(jo, true, SIZE_MAX);
      return true;
    } else if (jo->is_string()) {
      assert(tag);
      auto id = jo->get<std::string>();
      id = id.substr(1);
      *tag = std::stoi(id);
    } else {
      assert(false && "Expected object");
    }
    return false;
  }

  void endObject() override {
    assert(!nested_.empty());
    nested_.pop();
  }

  void beginArray(std::string_view name, size_t& count) override {}
  void endArray() override {}

  void read(std::string_view name, bool& v) override {}

  void read(std::string_view name, char& v) override {}

  void read(std::string_view name, int8_t& v) override {}
  void read(std::string_view name, int16_t& v) override {}
  void read(std::string_view name, int32_t& v) override {}
  void read(std::string_view name, int64_t& v) override {}

  void read(std::string_view name, uint8_t& v) override {}
  void read(std::string_view name, uint16_t& v) override {}
  void read(std::string_view name, uint32_t& v) override {}
  void read(std::string_view name, uint64_t& v) override {}

  void read(std::string_view name, float& v) override {}
  void read(std::string_view name, double& v) override {}

  void read(std::string_view name, std::string& v) override {}

 private:
  nlohmann::json root_;
  std::stack<std::tuple<nlohmann::json*, bool, size_t>> nested_;
};
} // namespace wut
