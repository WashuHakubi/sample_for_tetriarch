/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include "engine/renderables/mesh.h"
#include "engine/renderables/material.h"
#include "engine/renderer.h"

namespace ewok {
void Mesh::render(Renderer& renderer) {
  renderer.enqueueDraw(*this);
}
} // namespace ewok
