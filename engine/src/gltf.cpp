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
void loadAsset(Asset& asset, nlohmann::json const& jo) {
  asset.copyright = jo.contains("copyright") ? jo["copyright"].get<std::string>() : std::string{};
  asset.generator = jo.contains("generator") ? jo["generator"].get<std::string>() : std::string{};
  {
    auto versionStr = jo["version"].get<std::string_view>();
    char* endPos;
    asset.version.major = std::strtol(versionStr.data(), &endPos, 10);
    assert(*endPos == '.');
    asset.version.minor = std::strtol(endPos + 1, nullptr, 10);
  }

  if (jo.contains("minVersion")) {
    auto versionStr = jo["minVersion"].get<std::string_view>();
    char* endPos;
    asset.minVersion = Asset::Version{};
    asset.minVersion.value().major = std::strtol(versionStr.data(), &endPos, 10);
    assert(*endPos == '.');
    asset.minVersion.value().minor = std::strtol(endPos + 1, nullptr, 10);
  }
}
} // namespace

std::shared_ptr<GLTF> load(std::string_view jsonStr) {
  auto json = nlohmann::json::parse(jsonStr);
  auto result = std::make_shared<GLTF>();

  loadAsset(result->asset, json["asset"]);

  return nullptr;
}
} // namespace wut::gltf
