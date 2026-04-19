/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/component.h>
#include <wut/entity.h>
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

void readObject(IReader& reader, std::string_view name, Camera::Type& obj, ReadTags& tags) {
  static std::unordered_map<std::string, Camera::Type> nameToValue = {
      {"orthographic", Camera::Type::Orthographic},
      {"perspective", Camera::Type::Perspective},
  };
  std::string val;
  reader.read(name, val);
  obj = nameToValue.at(val);
}

void readObject(IReader& reader, std::string_view name, Mesh::Primitive::Mode& obj, ReadTags& tags) {
  uint32_t val;
  reader.read(name, val);
  obj = static_cast<Mesh::Primitive::Mode>(val);
}

void readObject(IReader& reader, std::string_view name, Sampler::Filter& obj, ReadTags& tags) {
  static std::unordered_map<std::string, Sampler::Filter> nameToValue = {
      {"NEAREST", Sampler::Filter::Nearest},
      {"LINEAR", Sampler::Filter::Linear},
      {"NEAREST_MIPMAP_NEAREST", Sampler::Filter::NearestMipmapNearest},
      {"LINEAR_MIPMAP_NEAREST", Sampler::Filter::LinearMipmapNearest},
      {"NEAREST_MIPMAP_LINEAR", Sampler::Filter::NearestMipmapLinear},
      {"LINEAR_MIPMAP_LINEAR", Sampler::Filter::LinearMipmapLinear},
  };

  std::string val;
  reader.read(name, val);
  obj = nameToValue.at(val);
}

void readObject(IReader& reader, std::string_view name, Sampler::Wrap& obj, ReadTags& tags) {
  static std::unordered_map<std::string, Sampler::Wrap> nameToValue = {
      {"CLAMP_TO_EDGE", Sampler::Wrap::ClampToEdge},
      {"MIRRORED_REPEAT", Sampler::Wrap::MirroedRepeat},
      {"REPEAT", Sampler::Wrap::Repeat},
  };

  std::string val;
  reader.read(name, val);
  obj = nameToValue.at(val);
}

void readObject(IReader& reader, std::string_view name, Material::AlphaMode& obj, ReadTags& tags) {
  static std::unordered_map<std::string, Material::AlphaMode> nameToValue = {
      {"OPAQUE", Material::AlphaMode::Opaque},
      {"MASK", Material::AlphaMode::Mask},
      {"BLEND", Material::AlphaMode::Blend},
  };
  std::string val;
  reader.read(name, val);
  obj = nameToValue.at(val);
}

std::shared_ptr<GLTF> parseGLTF(std::string_view jsonStr) {
  // auto json = nlohmann::json::parse(jsonStr);
  auto result = std::make_shared<GLTF>();

  auto reader = createJsonReader(jsonStr);

  read(*reader, *result);

  return result;
}

} // namespace wut::gltf
