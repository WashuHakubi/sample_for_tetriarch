/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/i_asset.h"
#include "engine/i_renderable.h"

namespace ewok {
class Mesh final : public IAsset, public IRenderable {
 public:
  Mesh(BufferHandle vertices, BufferHandle indices, MaterialPtr mat)
      : vertices_(vertices), indices_(indices), material_(std::move(mat)) {}

  void render(Renderer& renderer) override;

 private:
  friend class Renderer;

  BufferHandle vertices_;
  BufferHandle indices_;
  MaterialPtr material_;
};
} // namespace ewok
