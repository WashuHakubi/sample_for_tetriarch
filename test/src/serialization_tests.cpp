/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <catch2/catch_all.hpp>

#include <unordered_map>
#include <wut/serialization.h>

using namespace wut;

namespace {
struct S {
  int i;
  float f;
  std::string name;
  std::shared_ptr<S> other;
  std::shared_ptr<S> other2;
  std::vector<int> is;
  std::vector<std::shared_ptr<S>> others;

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_tuple("i", &S::i),
        std::make_tuple("f", &S::f),
        std::make_tuple("name", &S::name),
        std::make_tuple("other", &S::other),
        std::make_tuple("other2", &S::other2),
        std::make_tuple("is", &S::is),
        std::make_tuple("others", &S::others));
  }
};

struct TestWriter final : IWriter {
  bool beginObject(std::string_view name, void* tag = nullptr) override {
    buf += name;
    if (!name.empty()) {
      buf += "=";
    }

    if (tag == nullptr || tags.emplace(tag, lastTag).second) {
      if (tag != nullptr) {
        ++lastTag;
      }
      buf += "{";
      return true;
    } else {
      buf += "&" + std::to_string(tags[tag]);
      return false;
    }
  }

  void endObject() override { buf += "},"; }

  void beginArray(std::string_view name, size_t count) override {
    buf += name;
    buf += "=" + std::to_string(count) + "[";
  }

  void endArray() override { buf += "],"; }

  void write(std::string_view name, bool v) override {
    buf += name;
    buf += "=b:" + (v ? std::string("t") : std::string("f"));
    buf += ",";
  }

  void write(std::string_view name, int8_t v) override {
    buf += name;
    buf += "=i8:" + std::to_string(v);
    buf += ",";
  }

  void write(std::string_view name, int16_t v) override {
    buf += name;
    buf += "=i16:" + std::to_string(v);
    buf += ",";
  }

  void write(std::string_view name, int32_t v) override {
    buf += name;
    buf += "=i32:" + std::to_string(v);
    buf += ",";
  }

  void write(std::string_view name, int64_t v) override {
    buf += name;
    buf += "=i64:" + std::to_string(v);
    buf += ",";
  }

  void write(std::string_view name, uint8_t v) override {
    buf += name;
    buf += "=u8:" + std::to_string(v);
    buf += ",";
  }

  void write(std::string_view name, uint16_t v) override {
    buf += name;
    buf += "=u16:" + std::to_string(v);
    buf += ",";
  }

  void write(std::string_view name, uint32_t v) override {
    buf += name;
    buf += "=u32:" + std::to_string(v);
    buf += ",";
  }

  void write(std::string_view name, uint64_t v) override {
    buf += name;
    buf += "=u64:" + std::to_string(v);
    buf += ",";
  }

  void write(std::string_view name, float v) override {
    buf += name;
    buf += "=f32:" + std::to_string(v);
    buf += ",";
  }

  void write(std::string_view name, double v) override {
    buf += name;
    buf += "=f64:" + std::to_string(v);
    buf += ",";
  }

  void write(std::string_view name, std::string_view v) override {
    buf += name;
    buf += "=sv:\"";
    buf += v;
    buf += "\"";
    buf += ",";
  }

  auto toBuffer() const -> std::string override { return buf; }

  int lastTag{0};
  std::unordered_map<void*, int> tags;
  std::string buf;
};
} // namespace

TEST_CASE("Can serialize members type") {
  S s{
      .i = 42,
      .f = 3.14,
      .name = "stuff",
      .other = nullptr // no nested object
  };
  TestWriter w;

  writeObject(w, "", s);
  REQUIRE(w.buf == "{i=i32:42,f=f32:3.140000,name=sv:\"stuff\",is=0[],others=0[],},");

  s.other = std::make_shared<S>(10, 1.24, "other", nullptr);
  w = {};

  // Test nested object serialization works
  writeObject(w, "", s);
  REQUIRE(
      w.buf ==
      "{i=i32:42,f=f32:3.140000,name=sv:\"stuff\",other={i=i32:10,f=f32:1.240000,name=sv:\"other\",is=0[],others=0[],},is=0[],others=0[],},");

  w = {};
  s.other2 = s.other;

  // Ensure that serializing the same smart pointer twice results in a handle in the second case.
  writeObject(w, "", s);
  REQUIRE(
      w.buf ==
      "{i=i32:42,f=f32:3.140000,name=sv:\"stuff\",other={i=i32:10,f=f32:1.240000,name=sv:\"other\",is=0[],others=0[],},other2=&0is=0[],others=0[],},");
}

TEST_CASE("Json serialization") {
  S s{
      .i = 42,
      .f = 3.14f,
      .name = "stuff",
      .other = std::make_shared<S>(10, 1.24f, "other", nullptr),
      .is = {1, 2, 3, 4},
  };
  s.other2 = s.other;
  s.others.push_back(s.other);
  s.others.push_back(std::make_shared<S>(1, 2.0f, "asdf"));

  auto writer = createJsonWriter();
  writeObject(*writer, "", s);

  auto buf = writer->toBuffer();
  auto reader = createJsonReader(buf);

  S s2;
  readObject(*reader, "", s2);

  REQUIRE(s2.i == s.i);
  REQUIRE(s2.f == s.f);
  REQUIRE(s2.name == "stuff");
  REQUIRE(s2.is == std::vector<int>{1, 2, 3, 4});
  REQUIRE(s2.other != nullptr);
  REQUIRE(s2.other->i == s.other->i);
  REQUIRE(s2.other->f == s.other->f);
  REQUIRE(s2.other->name == "other");
  REQUIRE(s2.other->other == nullptr);
  REQUIRE(s2.other->other2 == nullptr);
  REQUIRE(s2.other2 == s2.other);
  REQUIRE(s2.others.size() == 2);
  REQUIRE(s2.others[0] == s2.other);
  REQUIRE(s2.others[1]->i == 1);
  REQUIRE(s2.others[1]->f == 2.0f);
  REQUIRE(s2.others[1]->name == "asdf");
}
