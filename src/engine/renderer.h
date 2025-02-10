/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"
#include "engine/math.h"
#include "engine/renderer_forward.h"

namespace ewok {
class Renderer {
 public:
  static auto create() -> std::shared_ptr<Renderer>;

  virtual ~Renderer() = default;

  void enqueueDraw(Mesh& mesh);

  void present();

 protected:
  Renderer() = default;

 private:
};

#ifndef NDEBUG
// Only available in debug builds
class DebugDrawer {
 public:
  void drawLine(Renderer& r, Vec3 color, Vec3 start, Vec3 end) {}
  void drawCircle(Renderer& r, Vec3 color, Vec3 center, float radius) {}
  void drawBox(Renderer& r, Vec3 color, Vec3 max, Vec3 min) {}
  void drawSphere(Renderer& r, Vec3 color, Vec3 center, float radius) {}
};
#endif

void registerRenderables(AssetDatabase& db);
} // namespace ewok
