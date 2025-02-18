/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include "engine/renderer.h"
#include "engine/asset_database.h"
#include "engine/renderables/material.h"
#include "engine/renderables/mesh.h"

#include "engine/loaders/material_loader.h"
#include "engine/loaders/mesh_loader.h"

namespace ewok {
struct DrawCommands {};

struct RenderCommand {
  ShaderHandle shader;
  BufferHandle constants;
  BufferHandle indices;
  BufferHandle vertices[4];
};

class RendererImpl : public Renderer {
 public:
  void scheduleDraw(RenderCommand cmd) {
    auto bucket = findBucket(cmd);
    commands_[bucket].push_back(cmd);
  }

  size_t findBucket(RenderCommand const& cmd) const {
    // Batching? Batching.
    return 0;
  }

  void present() {
    for (auto&& [bucketId, commands] : commands_) {
      // make like a tree and draw.
      (void)bucketId;
      (void)commands;
    }
  }

 private:
  std::unordered_map<size_t, std::vector<RenderCommand>> commands_;
};

auto Renderer::create() -> RendererPtr {
  return std::make_shared<RendererImpl>();
}

void Renderer::enqueueDraw(Mesh& mesh) {
  auto self = static_cast<RendererImpl*>(this);

  auto const& mat = *mesh.material_;
  RenderCommand cmd = {
      .shader = mat.shader_,
      .constants = mat.constants_,
      .indices = mesh.indices_,
      .vertices = {mesh.vertices_}};

  self->scheduleDraw(cmd);
}

void Renderer::present() {
  auto self = static_cast<RendererImpl*>(this);
  self->present();
}

void registerRenderables(AssetDatabase& db) {
  db.registerAssetLoader<Mesh>(std::make_unique<MeshLoader>());
  db.registerAssetLoader<Material>(std::make_unique<MaterialLoader>());
}
} // namespace ewok
