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
#include <vector>

#include <wut/serialization.h>

using namespace std::string_view_literals;

namespace wut::gltf {
enum class ComponentType {
  Unknown = 0,
  Byte = 5120,
  UnsignedByte = 5121,
  Short = 5122,
  UnsignedShort = 5123,
  UnsignedInt = 5125,
  Float = 5126,
};

void readObject(IReader& reader, std::string_view name, ComponentType& obj, ReadTags& tags);

enum class AccessorType {
  Scalar,
  Vec2,
  Vec3,
  Vec4,
  Mat2,
  Mat3,
  Mat4,
};

void readObject(IReader& reader, std::string_view name, AccessorType& obj, ReadTags& tags);

using MinMaxVar = std::array<float, 16>;

struct Accessor {
  struct Sparse {
    struct Indices {
      uint32_t bufferView;
      uint32_t byteOffset;
      ComponentType componentType;

      static auto serializeMembers() {
        using Type = Indices;
        return std::tuple{
            SERIALIZE_MEMBER(bufferView),
            SERIALIZE_MEMBER(byteOffset),
            SERIALIZE_MEMBER(componentType),
        };
      }
    };

    struct Values {
      uint32_t bufferView;
      uint32_t byteOffset;

      static auto serializeMembers() {
        using Type = Values;

        return std::tuple{
            SERIALIZE_MEMBER(bufferView),
            SERIALIZE_MEMBER(byteOffset),
        };
      }
    };

    uint32_t count;
    std::vector<Indices> indices;
    std::vector<Values> values;

    static auto serializeMembers() {
      using Type = Sparse;
      return std::tuple{
          SERIALIZE_MEMBER(count),
          SERIALIZE_MEMBER(indices),
          SERIALIZE_MEMBER(values),
      };
    }
  };

  std::optional<std::string> name;
  uint32_t bufferView{0};
  uint32_t byteOffset{0};
  ComponentType componentType;
  bool normalized{false};
  uint32_t count;
  AccessorType type;
  MinMaxVar max;
  MinMaxVar min;
  std::optional<Sparse> sparse;

  static auto serializeMembers() {
    using Type = Accessor;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(bufferView, DefaultValue<uint32_t>{INT_MAX}),
        SERIALIZE_MEMBER(byteOffset, DefaultValue<uint32_t>{0}),
        SERIALIZE_MEMBER(componentType),
        SERIALIZE_MEMBER(normalized, DefaultValue<bool>{false}),
        SERIALIZE_MEMBER(count),
        SERIALIZE_MEMBER(type),
        SERIALIZE_MEMBER(max),
        SERIALIZE_MEMBER(min),
        SERIALIZE_MEMBER(sparse),
    };
  }
};

void readObject(IReader& reader, std::string_view name, MinMaxVar& obj, ReadTags& tags);

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
    using Type = Asset;
    return std::tuple{
        SERIALIZE_MEMBER(version),
        SERIALIZE_MEMBER(generator),
        SERIALIZE_MEMBER(copyright),
    };
  }
};

void readObject(IReader& reader, std::string_view name, Asset::Version& obj, ReadTags& tags);

struct Buffer {
  std::optional<std::string> name;
  std::string uri;
  uint32_t byteLength;

  static auto serializeMembers() {
    using Type = Buffer;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(uri),
        SERIALIZE_MEMBER(byteLength),
    };
  }
};

struct BufferView {
  enum class TargetType {
    Unknown = 0,
    ArrayBuffer = 34962,
    ElementArrayBuffer = 34963,
  };

  std::optional<std::string> name;
  uint32_t buffer;
  uint32_t byteOffset{0};
  uint32_t byteLength;
  uint32_t byteStride{0};
  TargetType target{TargetType::Unknown};

  static auto serializeMembers() {
    using Type = BufferView;

    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(buffer),
        SERIALIZE_MEMBER(byteLength),
        SERIALIZE_MEMBER(byteOffset, DefaultValue<uint32_t>{0}),
        SERIALIZE_MEMBER(byteStride, DefaultValue<uint32_t>{0}),
        SERIALIZE_MEMBER(target, DefaultValue<TargetType>{TargetType::Unknown}),
    };
  }
};

void readObject(IReader& reader, std::string_view name, BufferView::TargetType& obj, ReadTags& tags);

struct Camera {
  enum class Type {
    Orthographic,
    Perspective,
  };

  struct Orthographic {
    float xMag;
    float yMag;
    float zFar;
    float zNear;

    static auto serializeMembers() {
      using Type = Orthographic;
      return std::tuple{
          SERIALIZE_MEMBER(xMag),
          SERIALIZE_MEMBER(yMag),
          SERIALIZE_MEMBER(zFar),
          SERIALIZE_MEMBER(zNear),
      };
    }
  };

  struct Perspective {
    float aspectRatio;
    float yFov;
    float zFar;
    float zNear;

    static auto serializeMembers() {
      using Type = Perspective;
      return std::tuple{
          SERIALIZE_MEMBER(aspectRatio),
          SERIALIZE_MEMBER(yFov),
          SERIALIZE_MEMBER(zFar),
          SERIALIZE_MEMBER(zNear),
      };
    }
  };

  std::optional<std::string> name;
  std::optional<Orthographic> orthographic;
  std::optional<Perspective> perspective;
  Type type;

  static auto serializeMembers() {
    using Type = Camera;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(orthographic),
        SERIALIZE_MEMBER(perspective),
        SERIALIZE_MEMBER(type),
    };
  }
};

void readObject(IReader& reader, std::string_view name, Camera::Type& obj, ReadTags& tags);

struct Image {
  enum class MimeType {
    Jpeg,
    Png,
  };

  std::optional<std::string> name;
  std::optional<std::string> uri;
  std::optional<MimeType> mimeType;
  std::optional<uint32_t> bufferView;

  static auto serializeMembers() {
    using Type = Image;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(uri),
        SERIALIZE_MEMBER(mimeType),
        SERIALIZE_MEMBER(bufferView),
    };
  }
};

void readObject(IReader& reader, std::string_view name, Image::MimeType& obj, ReadTags& tags);

// TODO:
struct Material {
  struct NormalTextureInfo {};
  struct OcclusionTextureInfo {};
  struct PBRMetallicRoughness {};
};

// TODO:
struct Mesh {
  struct Primitive {};
  std::optional<std::string> name;
};

struct Node {
  std::optional<std::string> name;
  std::optional<uint32_t> camera;
  std::optional<std::vector<uint32_t>> children;

  glm::mat4 matrix;

  std::optional<uint32_t> mesh;

  glm::quat rotation = {0, 0, 0, 1};

  glm::vec3 scale = {1, 1, 1};

  glm::vec3 translation = {0, 0, 0};

  static auto serializeMembers() {
    using Type = Node;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(camera),
        SERIALIZE_MEMBER(children),
        SERIALIZE_MEMBER(matrix, DefaultValue<glm::mat4>{glm::identity<glm::mat4>()}),
        SERIALIZE_MEMBER(mesh),
        SERIALIZE_MEMBER(rotation, DefaultValue<glm::quat>{{0, 0, 0, 1}}),
        SERIALIZE_MEMBER(scale, DefaultValue<glm::vec3>{{1, 1, 1}}),
        SERIALIZE_MEMBER(translation, DefaultValue<glm::vec3>{{0, 0, 0}}),
    };
  }
};

struct Sampler {
  enum class Filter {
    Nearest = 9728,
    Linear = 9729,
    NearestMipmapNearest = 9984,
    LinearMipmapNearest = 9985,
    NearestMipmapLinear = 9986,
    LinearMipmapLinear = 9987
  };

  enum class Wrap {
    ClampToEdge = 33071,
    MirroedRepeat = 33648,
    Repeat = 10497,
  };

  std::optional<std::string> name;
  Filter magFilter;
  Filter minFilter;
  Wrap wrapS{Wrap::Repeat};
  Wrap wrapT{Wrap::Repeat};

  static auto serializeMembers() {
    using Type = Sampler;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(magFilter),
        SERIALIZE_MEMBER(minFilter),
        SERIALIZE_MEMBER(wrapS, DefaultValue<Wrap>{Wrap::Repeat}),
        SERIALIZE_MEMBER(wrapT, DefaultValue<Wrap>{Wrap::Repeat})};
  }
};

void readObject(IReader& reader, std::string_view name, Sampler::Filter& obj, ReadTags& tags);

void readObject(IReader& reader, std::string_view name, Sampler::Wrap& obj, ReadTags& tags);

struct Scene {
  std::optional<std::string> name;
  std::vector<uint32_t> nodes;

  static auto serializeMembers() {
    using Type = Scene;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(nodes),
    };
  }
};

struct Texture {
  std::optional<std::string> name;
  std::optional<uint32_t> sampler;
  std::optional<uint32_t> source;

  static auto serializeMembers() {
    using Type = Texture;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(sampler),
        SERIALIZE_MEMBER(source),
    };
  }
};

struct TextureInfo {
  uint32_t index;
  uint32_t texCoord{0};

  static auto serializeMembers() {
    using Type = TextureInfo;
    return std::tuple{
        SERIALIZE_MEMBER(index),
        SERIALIZE_MEMBER(texCoord, DefaultValue<uint32_t>{0}),
    };
  }
};

struct GLTF {
  std::vector<Accessor> accessors;
  Asset asset;
  std::vector<Buffer> buffers;
  std::vector<BufferView> bufferViews;
  std::optional<std::vector<Camera>> cameras;
  std::optional<std::vector<Node>> nodes;
  std::optional<uint32_t> scene;
  std::optional<std::vector<Scene>> scenes;

  static auto serializeMembers() {
    using Type = GLTF;
    return std::tuple{
        SERIALIZE_MEMBER(accessors),
        SERIALIZE_MEMBER(asset),
        SERIALIZE_MEMBER(buffers),
        SERIALIZE_MEMBER(bufferViews),
        SERIALIZE_MEMBER(cameras),
        SERIALIZE_MEMBER(nodes),
        SERIALIZE_MEMBER(scene),
        SERIALIZE_MEMBER(scenes),
    };
  }
};

std::shared_ptr<GLTF> load(std::string_view json);
} // namespace wut::gltf
