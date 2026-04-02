/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/gltf.h>

#include <cassert>
#include <nlohmann/json.hpp>

namespace wut::gltf {
void readObject(IReader& reader, std::string_view name, ComponentType& obj, ReadTags& tags) {
  int val;
  reader.read(name, val);
  obj = static_cast<ComponentType>(val);
}
void readObject(IReader& reader, std::string_view name, AccessorType& obj, ReadTags& tags) {
  static std::unordered_map<std::string, AccessorType> nameToAccessor = {
      {"SCALAR", AccessorType::Scalar},
      {"VEC2", AccessorType::Vec2},
      {"VEC3", AccessorType::Vec3},
      {"VEC4", AccessorType::Vec4},
      {"MAT2", AccessorType::Mat2},
      {"MAT3", AccessorType::Mat3},
      {"MAT4", AccessorType::Mat4},
  };

  std::string val;
  reader.read(name, val);
  obj = nameToAccessor.at(val);
}

void readObject(IReader& reader, std::string_view name, MinMaxVar& obj, ReadTags& tags) {
  size_t count;
  reader.beginArray(name, count);
  assert(count <= obj.size());
  for (size_t i = 0; i < count; ++i) {
    float f;
    obj[i] = (reader.read("", f), f);
  }
  reader.endArray();
}
void readObject(IReader& reader, std::string_view name, Asset::Version& obj, ReadTags& tags) {
  std::string versionStr;
  reader.read(name, versionStr);
  char* endPos;
  obj.major = std::strtol(versionStr.data(), &endPos, 10);
  assert(*endPos == '.');
  obj.minor = std::strtol(endPos + 1, nullptr, 10);
}

void readObject(IReader& reader, std::string_view name, BufferView::TargetType& obj, ReadTags& tags) {
  int val;
  reader.read(name, val);
  obj = static_cast<BufferView::TargetType>(val);
}

std::shared_ptr<GLTF> load(std::string_view jsonStr) {
  // auto json = nlohmann::json::parse(jsonStr);
  auto result = std::make_shared<GLTF>();

  auto reader = createJsonReader(jsonStr);

  read(*reader, *result);

  return result;
}
} // namespace wut::gltf
