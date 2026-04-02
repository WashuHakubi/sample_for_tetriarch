/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <wut/serialization.h>

namespace wut::gltf {
enum class ComponentType {
  Byte = 5120,
  UnsignedByte = 5121,
  Short = 5122,
  UnsignedShort = 5123,
  UnsignedInt = 5125,
  Float = 5126,
};

enum class AccessorType {
  Scalar,
  Vec2,
  Vec3,
  Vec4,
  Mat2,
  Mat3,
  Mat4,
  String,
};

using MinMaxVar = std::variant<std::array<int, 16>, std::array<float, 16>>;

struct Accessor {
  struct Sparse {
    struct Indices {
      uint32_t bufferView;
      uint32_t byteOffset;
      ComponentType componentType;
    };

    struct Values {
      uint32_t bufferView;
      uint32_t byteOffset;
    };

    uint32_t count;
    std::vector<Indices> indices;
    std::vector<Values> values;
  };

  uint32_t bufferView{0};
  uint32_t byteOffset{0};
  ComponentType componentType;
  bool normalized{false};
  uint32_t count;
  AccessorType type;
  MinMaxVar max;
  MinMaxVar min;
  Sparse sparse;
};

struct Asset {
  struct Version {
    int major;
    int minor;
  };

  Version version;
  std::optional<Version> minVersion;
  std::optional<std::string> generator;
  std::optional<std::string> copyright;

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_tuple("version", &Asset::version),
        std::make_tuple("generator", &Asset::generator),
        std::make_tuple("copyright", &Asset::copyright));
  }
};

void readObject(IReader& reader, std::string_view name, Asset::Version& obj, ReadTags& tags);

struct Buffer {
  std::string uri;
  uint32_t byteLength;

  static auto serializeMembers() {
    return std::make_tuple(std::make_tuple("uri", &Buffer::uri), std::make_tuple("byteLength", &Buffer::byteLength));
  }
};

struct BufferView {
  enum class TargetType {
    Unknown = 0,
    ArrayBuffer = 34962,
    ElementArrayBuffer = 34963,
  };

  uint32_t buffer;
  uint32_t byteOffset{0};
  uint32_t byteLength;
  uint32_t byteStride{0};
  TargetType target{TargetType::Unknown};

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_tuple("buffer", &BufferView::buffer),
        std::make_tuple("byteLength", &BufferView::byteLength),
        std::make_tuple("byteOffset", &BufferView::byteOffset, DefaultValue<uint32_t>{0}),
        std::make_tuple("byteStride", &BufferView::byteStride, DefaultValue<uint32_t>{0}),
        std::make_tuple("target", &BufferView::target, DefaultValue<TargetType>{TargetType::Unknown}));
  }
};

void readObject(IReader& reader, std::string_view name, BufferView::TargetType& obj, ReadTags& tags);

struct GLTF {
  std::vector<Accessor> accessors;
  Asset asset;
  std::vector<Buffer> buffers;
  std::vector<BufferView> bufferViews;

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_tuple("asset", &GLTF::asset),
        std::make_tuple("buffers", &GLTF::buffers),
        std::make_tuple("bufferViews", &GLTF::bufferViews));
  }
};

std::shared_ptr<GLTF> load(std::string_view json);
} // namespace wut::gltf
