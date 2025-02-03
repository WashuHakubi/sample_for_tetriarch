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
#include "engine/system_file_provider.h"

#include "game/component/initial_scene_load_component.h"
#include "game/loaders/scene_loader.h"
#include "game/parsers/camera_parser.h"
#include "game/parsers/prefab_parser.h"

#include <filesystem>
#include <format>
#include <iostream>

using namespace ewok;

struct Mesh final : IAsset {};
struct MeshLoader : ITypedAssetLoader<Mesh> {
  auto loadAssetAsync(AssetDatabase& db, std::vector<char> data)
      -> concurrencpp::result<IAssetPtr> {
    auto mesh = std::make_shared<Mesh>();
    co_return mesh;
  }
};

class Model final : public IAsset {
 public:
  Model(std::vector<std::shared_ptr<Mesh>> meshes)
      : meshes_(std::move(meshes)) {}

 private:
  std::vector<std::shared_ptr<Mesh>> meshes_;
};

struct ModelLoader : ITypedAssetLoader<Model> {
  auto loadAssetAsync(AssetDatabase& db, std::vector<char> data)
      -> concurrencpp::result<IAssetPtr> {
    // pretend we loaded this from `data`
    std::vector<std::string> meshNames;
    std::vector<std::shared_ptr<Mesh>> meshes;
    for (auto&& meshName : meshNames) {
      auto meshPtr = co_await db.loadAssetAsync<Mesh>(meshName);
      meshes.push_back(meshPtr);
    }

    auto model = std::make_shared<Model>(std::move(meshes));
    co_return model;
  }
};

void prefix(int depth) {
  for (int i = 0; i < depth * 2; ++i) {
    std::cout << ' ';
  }
}

void print(GameObjectPtr const& go, int depth) {
  prefix(depth);
  std::cout << go->name() << std::endl;

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
  assetDatabase()->registerAssetLoader<Model>(std::make_unique<ModelLoader>());
  assetDatabase()->registerAssetLoader<Mesh>(std::make_unique<MeshLoader>());
  assetDatabase()->registerAssetLoader<Scene>(std::make_unique<SceneLoader>());

  // And register our component parsers
  assetDatabase()->registerComponentParser(std::make_shared<CameraParser>());
  assetDatabase()->registerComponentParser(std::make_shared<PrefabParser>());

  GameObjectPtr root = std::make_shared<GameObject>();

  // Fake game loop since I cannot be bothered to hook up SDL or something
  std::thread gameThread([&]() {
    constexpr float SimStepSize = 1.0f / 60.0f;
    constexpr auto SimStep =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::duration<double>(SimStepSize));
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
      // renderer->render();
    }

    executor->shutdown();
  });

  std::cout << "Press x to exit" << std::endl;
  while (true) {
    char c;
    std::cin.read(&c, 1);
    if (c == 'x')
      break;

    print(root, 0);
  }

  run = false;
  gameThread.join();

  return 0;
}
