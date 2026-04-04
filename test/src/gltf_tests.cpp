/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <catch2/catch_all.hpp>

#include <wut/gltf.h>

using namespace wut::gltf;

TEST_CASE("Can load GLTF json") {
  std::string data = R"({
  "scene" : 0,
  "scenes" : [
    {
      "nodes" : [ 0, 1]
    }
  ],
  "nodes" : [
    {
      "mesh" : 0
    },
    {
      "mesh" : 0,
      "translation" : [ 1.0, 0.0, 0.0 ]
    }
  ],
  
  "meshes" : [
    {
      "primitives" : [ {
        "attributes" : {
          "POSITION" : 1,
          "NORMAL" : 2
        },
        "indices" : 0
      } ]
    }
  ],

  "buffers" : [
    {
      "uri" : "SimpleMeshes.bin",
      "byteLength" : 80
    }
  ],
  "bufferViews" : [
    {
      "buffer" : 0,
      "byteOffset" : 0,
      "byteLength" : 6,
      "target" : 34963
    },
    {
      "buffer" : 0,
      "byteOffset" : 8,
      "byteLength" : 72,
      "byteStride" : 12,
      "target" : 34962
    }
  ],
  "accessors" : [
    {
      "bufferView" : 0,
      "byteOffset" : 0,
      "componentType" : 5123,
      "count" : 3,
      "type" : "SCALAR",
      "max" : [ 2 ],
      "min" : [ 0 ]
    },
    {
      "bufferView" : 1,
      "byteOffset" : 0,
      "componentType" : 5126,
      "count" : 3,
      "type" : "VEC3",
      "max" : [ 1.0, 1.0, 0.0 ],
      "min" : [ 0.0, 0.0, 0.0 ]
    },
    {
      "bufferView" : 1,
      "byteOffset" : 36,
      "componentType" : 5126,
      "count" : 3,
      "type" : "VEC3",
      "max" : [ 0.0, 0.0, 1.0 ],
      "min" : [ 0.0, 0.0, 1.0 ]
    }
  ],
  
  "asset" : {
    "version" : "2.0"
  }
})";

  auto result = load(data);

  REQUIRE(result->asset.version.major == 2);
  REQUIRE(result->asset.version.minor == 0);

  REQUIRE(result->buffers.size() == 1);
  REQUIRE(result->buffers[0].uri == "SimpleMeshes.bin");
  REQUIRE(result->buffers[0].byteLength == 80);

  REQUIRE(result->bufferViews.size() == 2);
  REQUIRE(result->bufferViews[0].buffer == 0);
  REQUIRE(result->bufferViews[0].byteOffset == 0);
  REQUIRE(result->bufferViews[0].byteLength == 6);
  REQUIRE(result->bufferViews[0].byteStride == 0);
  REQUIRE(result->bufferViews[0].target == BufferView::TargetType::ElementArrayBuffer);

  REQUIRE(result->bufferViews[1].buffer == 0);
  REQUIRE(result->bufferViews[1].byteOffset == 8);
  REQUIRE(result->bufferViews[1].byteLength == 72);
  REQUIRE(result->bufferViews[1].byteStride == 12);
  REQUIRE(result->bufferViews[1].target == BufferView::TargetType::ArrayBuffer);

  REQUIRE(result->accessors.size() == 3);
  REQUIRE(result->accessors[0].bufferView == 0);
  REQUIRE(result->accessors[0].byteOffset == 0);
  REQUIRE(result->accessors[0].componentType == ComponentType::UnsignedShort);
  REQUIRE(result->accessors[0].count == 3);
  REQUIRE(result->accessors[0].type == AccessorType::Scalar);
  REQUIRE(result->accessors[0].max[0] == 2);
  REQUIRE(result->accessors[0].min[0] == 0);

  REQUIRE(result->accessors[1].bufferView == 1);
  REQUIRE(result->accessors[1].byteOffset == 0);
  REQUIRE(result->accessors[1].componentType == ComponentType::Float);
  REQUIRE(result->accessors[1].count == 3);
  REQUIRE(result->accessors[1].type == AccessorType::Vec3);
  REQUIRE(result->accessors[1].max[0] == 1);
  REQUIRE(result->accessors[1].max[1] == 1);
  REQUIRE(result->accessors[1].max[2] == 0);
  REQUIRE(result->accessors[1].min[0] == 0);
  REQUIRE(result->accessors[1].min[1] == 0);
  REQUIRE(result->accessors[1].min[2] == 0);

  REQUIRE(result->accessors[2].bufferView == 1);
  REQUIRE(result->accessors[2].byteOffset == 36);
  REQUIRE(result->accessors[2].componentType == ComponentType::Float);
  REQUIRE(result->accessors[2].count == 3);
  REQUIRE(result->accessors[2].type == AccessorType::Vec3);
  REQUIRE(result->accessors[2].max[0] == 0);
  REQUIRE(result->accessors[2].max[1] == 0);
  REQUIRE(result->accessors[2].max[2] == 1);
  REQUIRE(result->accessors[2].min[0] == 0);
  REQUIRE(result->accessors[2].min[1] == 0);
  REQUIRE(result->accessors[2].min[2] == 1);

  REQUIRE(result->scene == 0);
  REQUIRE(result->scenes->size() == 1);
  REQUIRE(result->scenes->at(0).nodes.size() == 2);
  REQUIRE(result->scenes->at(0).nodes == std::vector<uint32_t>{0, 1});

  REQUIRE(result->nodes->size() == 2);
  REQUIRE(result->nodes->at(0).mesh == 0);
  REQUIRE(result->nodes->at(0).matrix == glm::identity<glm::mat4>());
  REQUIRE(result->nodes->at(0).rotation == glm::quat{0, 0, 0, 1});
  REQUIRE(result->nodes->at(0).scale == glm::vec3{1, 1, 1});
  REQUIRE(result->nodes->at(0).translation == glm::vec3{0, 0, 0});

  REQUIRE(result->nodes->at(1).mesh == 0);
  REQUIRE(result->nodes->at(1).matrix == glm::identity<glm::mat4>());
  REQUIRE(result->nodes->at(1).rotation == glm::quat{0, 0, 0, 1});
  REQUIRE(result->nodes->at(1).scale == glm::vec3{1, 1, 1});
  REQUIRE(result->nodes->at(1).translation == glm::vec3{1, 0, 0});

  REQUIRE(result->meshes->size() == 1);
  REQUIRE(result->meshes->at(0).primitives.size() == 1);
  REQUIRE(result->meshes->at(0).primitives[0].attributes.size() == 2);
  REQUIRE(result->meshes->at(0).primitives[0].attributes["POSITION"] == 1);
  REQUIRE(result->meshes->at(0).primitives[0].attributes["NORMAL"] == 2);
}
