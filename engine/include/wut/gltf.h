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
  std::optional<uint32_t> bufferView;
  uint32_t byteOffset{0};
  ComponentType componentType;
  bool normalized{false};
  uint32_t count;
  AccessorType type;
  std::optional<MinMaxVar> max;
  std::optional<MinMaxVar> min;
  std::optional<Sparse> sparse;

  static auto serializeMembers() {
    using Type = Accessor;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(bufferView),
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

// TODO:
struct Animation {
  struct Channel {
    struct Target {};
  };

  struct Sampler {};
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
  std::optional<std::string> uri;
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
    std::optional<float> aspectRatio;
    float yFov;
    std::optional<float> zFar;
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
  std::optional<std::string> name;
  std::optional<std::string> uri;
  std::optional<std::string> mimeType;
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

struct Material {
  enum class AlphaMode {
    Opaque,
    Mask,
    Blend,
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

  struct NormalTextureInfo : TextureInfo {
    float scale{1};

    static auto serializeMembers() {
      using Type = NormalTextureInfo;
      return detail::catMembers(TextureInfo::serializeMembers(), SERIALIZE_MEMBER(scale, DefaultValue<float>{1}));
    }
  };

  struct OcclusionTextureInfo : TextureInfo {
    float strength{1};

    static auto serializeMembers() {
      using Type = OcclusionTextureInfo;
      return detail::catMembers(TextureInfo::serializeMembers(), SERIALIZE_MEMBER(strength, DefaultValue<float>{1}));
    }
  };

  struct PBRMetallicRoughness {
    glm::vec4 baseColorFactor{1, 1, 1, 1};
    std::optional<TextureInfo> baseColorTexture;
    float metallicFactor{1};
    float roughnessFactor{1};
    std::optional<TextureInfo> metallicRoughnessTexture;

    static auto serializeMembers() {
      using Type = PBRMetallicRoughness;
      return std::tuple{
          SERIALIZE_MEMBER(baseColorFactor, DefaultValue<glm::vec4>{{1, 1, 1, 1}}),
          SERIALIZE_MEMBER(baseColorTexture),
          SERIALIZE_MEMBER(metallicFactor, DefaultValue<float>{1}),
          SERIALIZE_MEMBER(roughnessFactor, DefaultValue<float>{1}),
          SERIALIZE_MEMBER(metallicRoughnessTexture),
      };
    }
  };

  std::optional<std::string> name;
  std::optional<PBRMetallicRoughness> pbrMetallicRoughness;
  std::optional<NormalTextureInfo> normalTexture;
  std::optional<OcclusionTextureInfo> occlusionTexture;
  std::optional<TextureInfo> emissiveTexture;

  glm::vec3 emissiveFactor{0, 0, 0};
  float alphaCutoff{0.5f};
  AlphaMode alphaMode{AlphaMode::Opaque};
  bool doubleSided{false};

  static auto serializeMembers() {
    using Type = Material;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(pbrMetallicRoughness),
        SERIALIZE_MEMBER(normalTexture),
        SERIALIZE_MEMBER(occlusionTexture),
        SERIALIZE_MEMBER(emissiveTexture),
        SERIALIZE_MEMBER(emissiveFactor, DefaultValue<glm::vec3>{{0, 0, 0}}),
        SERIALIZE_MEMBER(alphaCutoff, DefaultValue<float>{0.5f}),
        SERIALIZE_MEMBER(alphaMode, DefaultValue<AlphaMode>{AlphaMode::Opaque}),
        SERIALIZE_MEMBER(doubleSided, DefaultValue<bool>{false}),
    };
  }
};

void readObject(IReader& reader, std::string_view name, Material::AlphaMode& obj, ReadTags& tags);

struct Mesh {
  struct Primitive {
    enum class Mode {
      Points = 0,
      Lines = 1,
      LineLoop = 2,
      LineStrip = 3,
      Triangles = 4,
      TriangleStrip = 5,
      TriangleFan = 6,
    };

    std::unordered_map<std::string, uint32_t> attributes;
    std::optional<uint32_t> indices;
    std::optional<uint32_t> material;
    Mode mode{Mode::Triangles};
    std::optional<std::vector<std::unordered_map<std::string, uint32_t>>> targets;

    static auto serializeMembers() {
      using Type = Mesh::Primitive;
      return std::tuple{
          SERIALIZE_MEMBER(attributes),
          SERIALIZE_MEMBER(indices),
          SERIALIZE_MEMBER(material),
          SERIALIZE_MEMBER(mode, DefaultValue<Mode>{Mode::Triangles}),
          SERIALIZE_MEMBER(targets),
      };
    }
  };

  std::optional<std::string> name;
  std::vector<Primitive> primitives;
  std::optional<std::vector<float>> weights;

  static auto serializeMembers() {
    using Type = Mesh;
    return std::tuple{
        SERIALIZE_MEMBER(name),
        SERIALIZE_MEMBER(primitives),
        SERIALIZE_MEMBER(weights),
    };
  }
};

void readObject(IReader& reader, std::string_view name, Mesh::Primitive::Mode& obj, ReadTags& tags);

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
  std::optional<Filter> magFilter;
  std::optional<Filter> minFilter;
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

// TODO:
struct Skin {};

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

struct GLTF {
  std::optional<std::vector<Accessor>> accessors;
  Asset asset;
  std::optional<std::vector<Buffer>> buffers;
  std::optional<std::vector<BufferView>> bufferViews;
  std::optional<std::vector<Camera>> cameras;
  std::optional<std::vector<Material>> materials;
  std::optional<std::vector<Mesh>> meshes;
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
        SERIALIZE_MEMBER(materials),
        SERIALIZE_MEMBER(meshes),
        SERIALIZE_MEMBER(nodes),
        SERIALIZE_MEMBER(scene),
        SERIALIZE_MEMBER(scenes),
    };
  }
};

std::shared_ptr<GLTF> parseGLTF(std::string_view json);
} // namespace wut::gltf
