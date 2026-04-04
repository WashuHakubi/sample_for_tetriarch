/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/serialization.h>

#include <stack>
#include <nlohmann/json.hpp>

namespace wut {
struct JsonWriter final : IWriter {
  JsonWriter() : root_() {}

  void writeHandle(std::string_view name, int tag) override {
    if (tag == -1) {
      return;
    }

    auto& [cur, isObj] = nested_.top();
    if (isObj) {
      (*cur)[name] = "&" + std::to_string(tag);
    } else {
      cur->push_back("&" + std::to_string(tag));
    }
  }

  void beginObject(std::string_view name) override {
    assert(root_.is_null() || root_.is_object());
    if (root_.is_null()) {
      root_ = nlohmann::json::object();
      nested_.emplace(&root_, true);
      if (name.empty()) {
        return;
      }
    }

    auto& [cur, isObj] = nested_.top();
    if (isObj) {
      auto& jo = (*cur)[name] = nlohmann::json::object();
      nested_.emplace(&jo, true);
    } else {
      cur->push_back(nlohmann::json::object());
      nested_.emplace(&cur->back(), true);
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

  void write(std::string_view name, std::nullptr_t v) override { writeEntry(name, nullptr); }
  auto toBuffer() const -> std::string override { return root_.dump(2); }

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

  nlohmann::json root_;
  std::stack<std::tuple<nlohmann::json*, bool>> nested_;
};

std::shared_ptr<IWriter> createJsonWriter() {
  return std::make_shared<JsonWriter>();
}

struct JsonReader final : IReader {
  JsonReader(std::string_view data) { root_ = nlohmann::json::parse(data); }

  bool has(std::string_view name) override {
    assert(root_.is_object());
    if (nested_.empty()) {
      return root_.contains(name);
    }

    // must be an object.
    assert(std::get<1>(nested_.top()));
    return std::get<0>(nested_.top())->contains(name);
  }

  bool readHandle(std::string_view name, int& tag) override {
    if (nested_.empty()) {
      assert(root_.is_object());
      nested_.emplace(&root_, true, SIZE_MAX);

      // INT_MAX to indicate tag is unused.
      tag = INT_MAX;
      return false;
    }

    auto& [cur, isObj, idx] = nested_.top();
    nlohmann::json::value_type* jo;
    if (isObj) {
      if (!cur->contains(name)) {
        // -1 tag indicates missing object.
        tag = -1;
        return true;
      }

      jo = &cur->at(name);
    } else {
      if (idx >= cur->size()) {
        tag = -1;
        return true;
      }
      jo = &cur->at(idx);
    }

    if (jo->is_object()) {
      return false;
    } else if (jo->is_string()) {
      auto id = jo->get<std::string>();
      id = id.substr(1);
      tag = std::stoi(id);
    } else {
      assert(false && "Expected object or string");
    }

    if (!isObj) {
      ++idx;
    }

    return true;
  }

  void enumerate(std::function<void(IReader& reader, std::string_view name)> const& fn) override {
    assert(!nested_.empty());
    auto [cur, isObj, id] = nested_.top();
    assert(isObj);

    for (auto&& [k, _] : cur->items()) {
      fn(*this, k);
    }
  }

  void beginObject(std::string_view name) override {
    if (nested_.empty()) {
      assert(root_.is_object());
      nested_.emplace(&root_, true, SIZE_MAX);
      return;
    }

    auto& [cur, isObj, idx] = nested_.top();
    nlohmann::json::value_type* jo;
    if (isObj) {
      jo = &cur->at(name);
    } else {
      jo = &cur->at(idx++);
    }

    assert(jo->is_object());
    nested_.emplace(jo, true, SIZE_MAX);
    return;
  }

  void endObject() override {
    assert(!nested_.empty());
    nested_.pop();
  }

  void beginArray(std::string_view name, size_t& count) override {
    assert(root_.is_object());
    auto& [cur, isObj, idx] = nested_.top();

    // Nested arrays are not supported
    assert(isObj);

    auto& jo = (*cur)[name];
    assert(jo.is_array());
    count = jo.size();
    nested_.emplace(&jo, false, 0);
  }
  void endArray() override { endObject(); }

  void read(std::string_view name, bool& v) override { readInternal(name, v); }

  void read(std::string_view name, int8_t& v) override { readInternal(name, v); }
  void read(std::string_view name, int16_t& v) override { readInternal(name, v); }
  void read(std::string_view name, int32_t& v) override { readInternal(name, v); }
  void read(std::string_view name, int64_t& v) override { readInternal(name, v); }

  void read(std::string_view name, uint8_t& v) override { readInternal(name, v); }
  void read(std::string_view name, uint16_t& v) override { readInternal(name, v); }
  void read(std::string_view name, uint32_t& v) override { readInternal(name, v); }
  void read(std::string_view name, uint64_t& v) override { readInternal(name, v); }

  void read(std::string_view name, float& v) override { readInternal(name, v); }
  void read(std::string_view name, double& v) override { readInternal(name, v); }

  void read(std::string_view name, std::string& v) override { readInternal(name, v); }

  bool read(std::string_view name, std::nullptr_t) override {
    auto& [cur, isObj, idx] = nested_.top();

    nlohmann::json::value_type* v;
    if (isObj) {
      v = &cur->at(name);
    } else {
      v = &cur->at(idx);
    }

    if (v->is_null() && !isObj) {
      // If we're an array, and the entry is null then we need to move to the next object.
      ++idx;
    }

    return v->is_null();
  }

 private:
  template <class T>
  void readInternal(std::string_view name, T& v) {
    auto& [cur, isObj, idx] = nested_.top();
    assert(isObj || name.empty());

    if (isObj) {
      v = (*cur)[name];
    } else {
      v = cur->at(idx++);
    }
  }
  nlohmann::json root_;
  std::stack<std::tuple<nlohmann::json*, bool, size_t>> nested_;
};

std::shared_ptr<IReader> createJsonReader(std::string_view data) {
  return std::make_shared<JsonReader>(data);
}

void readObject(IReader& reader, std::string_view name, glm::vec2& obj, ReadTags& tags) {
  size_t count;
  reader.beginArray(name, count);
  assert(count == 2);

  reader.read("", obj.x);
  reader.read("", obj.y);
  reader.endArray();
}

void writeObject(IWriter& writer, std::string_view name, const glm::vec2& obj, WriteTags& tags) {
  writer.beginArray(name, 2);
  writer.write("", obj.x);
  writer.write("", obj.y);
  writer.endArray();
}

void readObject(IReader& reader, std::string_view name, glm::vec3& obj, ReadTags& tags) {
  size_t count;
  reader.beginArray(name, count);
  assert(count == 3);

  reader.read("", obj.x);
  reader.read("", obj.y);
  reader.read("", obj.z);
  reader.endArray();
}

void writeObject(IWriter& writer, std::string_view name, const glm::vec3& obj, WriteTags& tags) {
  writer.beginArray(name, 3);
  writer.write("", obj.x);
  writer.write("", obj.y);
  writer.write("", obj.z);
  writer.endArray();
}

void readObject(IReader& reader, std::string_view name, glm::vec4& obj, ReadTags& tags) {
  size_t count;
  reader.beginArray(name, count);
  assert(count == 4);

  reader.read("", obj.x);
  reader.read("", obj.y);
  reader.read("", obj.z);
  reader.read("", obj.w);
  reader.endArray();
}

void writeObject(IWriter& writer, std::string_view name, const glm::vec4& obj, WriteTags& tags) {
  writer.beginArray(name, 4);
  writer.write("", obj.x);
  writer.write("", obj.y);
  writer.write("", obj.z);
  writer.write("", obj.w);
  writer.endArray();
}

void readObject(IReader& reader, std::string_view name, glm::quat& obj, ReadTags& tags) {
  size_t count;
  reader.beginArray(name, count);
  assert(count == 4);

  reader.read("", obj.x);
  reader.read("", obj.y);
  reader.read("", obj.z);
  reader.read("", obj.w);
  reader.endArray();
}

void writeObject(IWriter& writer, std::string_view name, const glm::quat& obj, WriteTags& tags) {
  writer.beginArray(name, 4);
  writer.write("", obj.x);
  writer.write("", obj.y);
  writer.write("", obj.z);
  writer.write("", obj.w);
  writer.endArray();
}

void readObject(IReader& reader, std::string_view name, glm::mat4x4& obj, ReadTags& tags) {
  size_t count;
  reader.beginArray(name, count);
  assert(count == 16);
  // We store matricies in column major order (that is, [C0R0, C0R1, C0R2, C0R3, C1R0, ...])
  for (auto col = 0u; col < 4; ++col) {
    for (auto row = 0u; row < 4; ++row) {
      reader.read("", obj[col][row]);
    }
  }
  reader.endArray();
}

void writeObject(IWriter& writer, std::string_view name, const glm::mat4x4& obj, WriteTags& tags) {
  writer.beginArray(name, 16);
  for (auto col = 0u; col < 4; ++col) {
    for (auto row = 0u; row < 4; ++row) {
      writer.write("", obj[col][row]);
    }
  }
  writer.endArray();
}

} // namespace wut
