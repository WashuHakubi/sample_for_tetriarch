/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "ecs_systems.h"

// Systems
#include "axis_debug_system.h"
#include "debug_cube_system.h"
#include "frame_rate_system.h"
#include "orbit_camera_system.h"
#include "sample_terrain_system.h"

namespace ew {
void EcsSystems::clear() {
  updateSystems_.clear();
  renderSystems_.clear();
  messageHandlers_.clear();
  typeToSystem_.clear();

  // Remove the systems in the opposite order they were added, this should ensure that any ordering related destruction
  // semantics are respected.
  while (!systems_.empty()) {
    systems_.pop_back();
  }
}

void EcsSystems::render(float dt) const {
  for (auto&& system : renderSystems_) {
    system(dt);
  }
}

void EcsSystems::update(float dt) const {
  for (auto&& system : updateSystems_) {
    system(dt);
  }
}

void EcsSystems::handleMessage(GameThreadMsg const& msg) const {
  for (auto&& system : messageHandlers_) {
    system(msg);
  }
}

std::shared_ptr<EcsSystems> EcsSystems::create(IApplicationPtr const& app, IAssetProviderPtr const& assetProvider) {
  auto systems = std::make_shared<EcsSystems>(InternalOnly{});

  // Register all systems
  auto registry = systems->addSystem<entt::registry>();
  systems->addSystem<FrameRateSystem>();
  systems->addSystem<AxisDebugSystem>(assetProvider, registry);

  auto terrain = systems->addSystem<SampleTerrainSystem>(app->updateScheduler(), assetProvider, registry);
  systems->addSystem<DebugCubeSystem>(assetProvider, registry);
  systems->addSystem<ew::OrbitCameraSystem>(app, registry, terrain);

  return systems;
}
} // namespace ew
