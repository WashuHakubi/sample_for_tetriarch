/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/gltf.h>

#include <cassert>
#include <nlohmann/json.hpp>

namespace wut::gltf {
namespace {
void loadAccessor(Accessor& accessor, nlohmann::json& jo) {
  accessor.bufferView = jo.contains("bufferView") ? jo["bufferView"].get<uint32_t>() : 0;
  accessor.byteOffset = jo.contains("byteOffset") ? jo["byteOffset"].get<uint32_t>() : 0;
  accessor.componentType = static_cast<ComponentType>(jo["componentType"].get<uint32_t>());
  accessor.normalized = jo.contains("normalized") ? jo["normalized"].get<bool>() : false;
  accessor.count = jo["count"];

  if (jo.contains("sparse")) {
    auto& joSparse = jo["sparse"];
    auto& sparse = accessor.sparse;
    sparse.count = joSparse["count"];

    for (auto&& joIndices : joSparse["indices"]) {
      sparse.indices.push_back({});
      auto& index = sparse.indices.back();
      index.bufferView = joIndices["bufferView"];
      index.byteOffset = joIndices.contains("byteOffset") ? joIndices["byteOffset"].get<uint32_t>() : 0u;
      index.componentType = static_cast<ComponentType>(jo["componentType"].get<uint32_t>());
    }

    for (auto&& joValues : joSparse["values"]) {
      sparse.values.push_back({});
      auto& value = sparse.values.back();
      value.bufferView = joValues["bufferView"];
      value.byteOffset = joValues.contains("byteOffset") ? joValues["byteOffset"].get<uint32_t>() : 0u;
    }
  }
}
} // namespace

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
