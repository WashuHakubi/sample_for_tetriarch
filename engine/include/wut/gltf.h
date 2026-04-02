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
      ComponentType type;
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
};

struct Asset {
  struct Version {
    int major;
    int minor;
  };

  Version version;
  std::optional<Version> minVersion;
  std::string generator;
  std::string copyright;
};

struct Buffer {
  std::string uri;
  uint32_t byteLength;
};

struct BufferView {
  enum class TargetType {
    ArrayBuffer = 34962,
    ElementArrayBuffer = 34963,
  };

  uint32_t buffer;
  uint32_t byteOffset{0};
  uint32_t byteLength;
  uint32_t byteStride;
  TargetType target;
};

struct GLTF {
  std::vector<Accessor> accessors;
  Asset asset;
  std::vector<Buffer> buffers;
  std::vector<BufferView> bufferViews;
};

std::shared_ptr<GLTF> load(std::string_view json);
} // namespace wut::gltf
