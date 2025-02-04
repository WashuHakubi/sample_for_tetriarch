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
class Material final : public IAsset {
 public:
  void bind(Renderer& renderer);

 private:
  friend class Renderer;

  BufferHandle constants_;
  ShaderHandle shader_;
};
} // namespace ewok
