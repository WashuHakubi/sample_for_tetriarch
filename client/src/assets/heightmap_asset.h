/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "i_asset.h"

struct HeightmapAsset final : ew::Asset<HeightmapAsset> {
  HeightmapAsset(int width, int height, int channels, unsigned char* data);

  ~HeightmapAsset() override;

  constexpr int width() const { return width_; }

  constexpr int height() const { return height_; }

  constexpr unsigned char sample(int w, int h) const { return *(data_ + (w + h * width_) * channels_); }

 private:
  unsigned char* data_;
  int width_;
  int height_;
  int channels_;
};
