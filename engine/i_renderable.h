/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"
#include "engine/renderer_forward.h"

namespace ewok {
struct IRenderable {
  virtual ~IRenderable() = default;

  virtual void render(Renderer& renderer) = 0;
};
using IRenderablePtr = std::shared_ptr<IRenderable>;

} // namespace ewok
