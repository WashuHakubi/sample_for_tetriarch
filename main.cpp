/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "engine/asset_database.h"
#include "engine/component.h"
#include "engine/game_object.h"
#include "engine/null_file_provider.h"
#include "engine/renderables/mesh.h"
#include "engine/renderer.h"
#include "engine/system_file_provider.h"

#include "game/component/initial_scene_load_component.h"
#include "game/loaders/scene_loader.h"
#include "game/parsers/camera_parser.h"
#include "game/parsers/prefab_parser.h"

#include <filesystem>
#include <format>
#include <iostream>

using namespace ewok;

class Model final : public IAsset, public Component<Model> {
 public:
  void render(Renderer& renderer) override {
    for (auto&& mesh : meshes_) {
      mesh->render(renderer);
    }
  }

 private:
  std::vector<std::shared_ptr<Mesh>> meshes_;
};

class ModelLoader : ITypedAssetLoader<Model> {
 public:
  auto loadAssetAsync(AssetDatabase& db, std::vector<char> data)
      -> concurrencpp::result<IAssetPtr> override {
    co_return std::make_shared<Model>();
  }
};

// Stupid, but it works, add an indent.
void indent(int depth) {
  for (int i = 0; i < depth * 2; ++i) {
    std::cout << ' ';
  }
}

// Just print the tree of game objects.
void print(GameObjectPtr const& go, int depth) {
  indent(depth);
  std::cout << go->name() << " (" << go.get() << ")" << std::endl;

  for (auto&& comp : go->components()) {
    indent(depth);
    std::cout << comp->describe() << " " << std::endl;
  }

  for (auto&& child : go->children()) {
    print(child, depth + 1);
  }
}

int main() {
  concurrencpp::runtime runtime;
  auto executor = runtime.make_manual_executor();
  setGlobalExecutor(executor);

  auto ioExecutor = runtime.background_executor();
  std::atomic_bool run{true};

  setAssetDatabase(std::make_shared<AssetDatabase>(
      std::make_shared<SystemFileProvider>(ioExecutor, "assets"), executor));

  // Register our loaders
  registerRenderables(*assetDatabase());
  assetDatabase()->registerAssetLoader<Scene>(std::make_unique<SceneLoader>());

  // And register our component parsers
  assetDatabase()->registerComponentParser(std::make_shared<CameraParser>());
  assetDatabase()->registerComponentParser(std::make_shared<PrefabParser>());

  // We're only creating this outside of the thread for the print() method.
  // Otherwise it would be in the thread.
  GameObjectPtr root = std::make_shared<GameObject>();

  // Fake game loop since I cannot be bothered to hook up SDL or something
  std::thread gameThread([&]() {
    constexpr float SimStepSize = 1.0f / 60.0f;
    constexpr auto SimStep =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::duration<double>(SimStepSize));

    auto renderer = Renderer::create();

    // Startup the game by loading the initial scene.
    root->addComponent(std::make_shared<InitialSceneLoadComponent>());

    auto last = std::chrono::system_clock::now();
    while (run) {
      auto cur = std::chrono::system_clock::now();
      // Compute a delta since the last sim tick
      auto dur =
          std::chrono::duration_cast<std::chrono::milliseconds>(cur - last);

      if (dur >= SimStep) {
        // Sim tick will run, so update our last time.
        last = cur;
      }

      // Run sim ticks for as long as we need to. We might actually want to
      // render some graphics frames in here if dur is particularily large.
      while (dur >= SimStep) {
        dur -= SimStep;
        root->update(SimStepSize);
        root->postUpdate();

        executor->loop(executor->size());
      }

      // do render.
      renderer->present();
    }

    executor->shutdown();
  });

  std::cout << "Press x to exit" << std::endl;
  while (true) {
    char c;
    std::cin.read(&c, 1);
    if (c == 'x')
      break;

    // Technically not very thread safe.
    print(root, 0);
  }

  run = false;
  gameThread.join();

  return 0;
}
