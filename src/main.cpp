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
#include "engine/object_database.h"
#include "engine/renderables/mesh.h"
#include "engine/renderer.h"
#include "engine/system_file_provider.h"

#include "game/component/initial_scene_load_component.h"
#include "game/loaders/scene_loader.h"

#include "editor/editor.h"

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"
#include "imgui.h"

#include "rml_context.h"

#include <filesystem>
#include <format>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>

#include <ng-log/logging.h>

using namespace ewok;

constexpr uint32_t windowStartWidth = 1280;
constexpr uint32_t windowStartHeight = 720;

class RootGameObject : public GameObject {
 public:
  static auto create(Guid id, bool lazyAttach = false) -> std::shared_ptr<RootGameObject> {
    return std::make_shared<RootGameObject>(ProtectedOnly{}, id, lazyAttach);
  }

  using GameObject::GameObject;

  void doUpdate(float dt) {
    if (!once_) {
      once_ = true;
      fireAttached();
    }

    update(dt);
    postUpdate();
  }

 private:
  bool once_{false};
};

struct PositionColorVertex {
  Vec3 position;
  UInt8Vec4 color;
};

struct AppState {
  concurrencpp::runtime runtime;
  std::shared_ptr<concurrencpp::manual_executor> executor;

  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> sdlWindow;
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> sdlRenderer;

  Rml::DataModelHandle rmlModel;
  std::unique_ptr<RmlContext> rmlContext;
  std::shared_ptr<Renderer> renderer;
  std::shared_ptr<RootGameObject> root;
  std::shared_ptr<Editor> editor;

  int frame{0};
  bool run{true};
  uint64_t prevTime{};
};

void initializeEngine(AppState* appState) {
  auto ioExecutor = appState->runtime.background_executor();
  appState->executor = appState->runtime.make_manual_executor();
  setGlobalExecutor(appState->executor);

  auto sfp = std::make_shared<SystemFileProvider>(ioExecutor, "assets");
  setAssetDatabase(std::make_shared<AssetDatabase>(sfp, appState->executor));

  setObjectDatabase(std::make_shared<ObjectDatabase>());

  appState->renderer = Renderer::create();

  appState->editor = std::make_shared<Editor>();

  // Register our loaders
  registerRenderables(*assetDatabase());
  assetDatabase()->registerAssetLoader<Scene>(std::make_unique<SceneLoader>());

  // We're only creating this outside of the thread for the print() method.
  // Otherwise it would be in the thread.
  appState->root = RootGameObject::create(Guid{}, true /* lazyAttach */);

  // Startup the game by loading the initial scene.
  appState->root->addComponent(std::make_shared<InitialSceneLoadComponent>());

  appState->rmlModel = appState->rmlContext->bind("sampleBindings", [&](Rml::DataModelConstructor& constructor) {
    constructor.BindEventCallback("doQuit", [appState](Rml::DataModelHandle, Rml::Event& evt, Rml::VariantList const&) {
      appState->run = false;
    });
    constructor.Bind("frameNumber", &appState->frame);
  });

  appState->rmlContext->loadDocument("rml/test.rml");
}

SDL_AppResult SDL_Fail() {
  SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
  return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  FLAGS_logtostderr = 1;
  nglog::InitializeLogging(argv[0]);

  if (not SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    return SDL_Fail();
  }

  SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

  const float windowSizeScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "SDL Minimal Sample");
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, false);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, int(windowStartWidth * windowSizeScale));
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, int(windowStartHeight * windowSizeScale));
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
  auto window = SDL_CreateWindowWithProperties(props);
  SDL_DestroyProperties(props);

  if (not window) {
    return SDL_Fail();
  }

  auto renderer = SDL_CreateRenderer(window, "opengl");

  auto appState = new AppState{
      .sdlWindow = {window, &SDL_DestroyWindow},
      .sdlRenderer = {renderer, &SDL_DestroyRenderer},
      .rmlContext = std::make_unique<RmlContext>(window, renderer, std::make_pair(windowStartWidth, windowStartHeight)),
  };
  initializeEngine(appState);

  *appstate = appState;
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  auto* app = static_cast<AppState*>(appstate);
  if (app) {
    delete app;
  }

  SDL_Quit();
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  auto* app = static_cast<AppState*>(appstate);

  if (event->type == SDL_EVENT_QUIT) {
    app->run = false;
  }

  if (!app->rmlContext->processEvent(*event)) {
    // Event was not handled by RML, so we should handle it.
  }

  return app->run ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  constexpr float kSimTickTime = 1.0f / 60.0f;
  auto* app = reinterpret_cast<AppState*>(appstate);

  ++app->frame;
  app->rmlModel.DirtyAllVariables();

  auto time = SDL_GetTicks();
  auto delta = (time - app->prevTime) / 1000.0f;
  auto root = app->root;

  if (delta >= kSimTickTime) {
    app->prevTime = time;

    // Run sim ticks for as long as we need to. We might actually want to
    // render some graphics frames in here if delta is particularily large.
    do {
      // Update all GOs
      root->doUpdate(kSimTickTime);

      // Run any pending coroutines
      app->executor->loop(app->executor->size());

      delta -= kSimTickTime;
    } while (delta >= kSimTickTime);
  }

  // We render every time we iterate.
  root->render(*app->renderer, delta);
  app->renderer->present();

  app->rmlContext->render();
  SDL_RenderPresent(app->sdlRenderer.get());

  return app->run ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}
