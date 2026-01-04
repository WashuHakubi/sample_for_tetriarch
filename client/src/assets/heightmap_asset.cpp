/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "heightmap_asset.h"
#include <stb/stb_image.h>

HeightmapAsset::HeightmapAsset(int width, int height, int channels, unsigned char* data)
    : data_(data)
    , width_(width)
    , height_(height)
    , channels_(channels) {}

HeightmapAsset::~HeightmapAsset() {
  stbi_image_free(data_);
}

auto HeightmapAssetLoader::load(ew::AssetProviderPtr const& provider, const std::string& fn, std::string const& data)
    -> ew::IAssetPtr {
  int width;
  int height;
  int channels;

  auto const imageData = stbi_load_from_memory(
      reinterpret_cast<stbi_uc const*>(data.data()),
      static_cast<int>(data.size()),
      &width,
      &height,
      &channels,
      0);

  return std::make_shared<HeightmapAsset>(width, height, channels, imageData);
}