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
#include "game/parsers/camera_parser.h"
#include "game/parsers/prefab_parser.h"

#include <filesystem>
#include <format>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

using namespace ewok;

constexpr uint32_t windowStartWidth = 1280;
constexpr uint32_t windowStartHeight = 720;

class RootGameObject : public GameObject {
 public:
  static auto create(Guid id, bool lazyAttach = false)
      -> std::shared_ptr<RootGameObject> {
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

struct AppState {
  concurrencpp::runtime runtime;
  std::shared_ptr<concurrencpp::manual_executor> executor;

  SDL_Window* sdlWindow;

  std::shared_ptr<Renderer> renderer;
  std::shared_ptr<RootGameObject> root;

  uint16_t width;
  uint16_t height;

  bool run{true};
  uint64_t prevTime{};
};

// Just print the tree of game objects.
void print(GameObjectPtr const& go, int depth, int& line) {
  auto indent = depth * 2;
  auto s =
      std::format("{} ({})", go->name(), reinterpret_cast<size_t>(go.get()));
  bgfx::dbgTextPrintf(indent, line, 0x0F, s.c_str());

  for (auto&& comp : go->components()) {
    // indent(out, depth);
    // out << comp->describe() << " " << std::endl;
  }

  for (auto&& child : go->children()) {
    print(child, depth + 1, ++line);
  }
}

void initializeEngine(AppState* appState) {
  auto ioExecutor = appState->runtime.background_executor();
  appState->executor = appState->runtime.make_manual_executor();
  setGlobalExecutor(appState->executor);

  setAssetDatabase(std::make_shared<AssetDatabase>(
      std::make_shared<SystemFileProvider>(ioExecutor, "assets"),
      appState->executor));

  setObjectDatabase(std::make_shared<ObjectDatabase>());

  appState->renderer = Renderer::create();

  // Register our loaders
  registerRenderables(*assetDatabase());
  assetDatabase()->registerAssetLoader<Scene>(std::make_unique<SceneLoader>());

  // And register our component parsers
  assetDatabase()->registerComponentParser(std::make_shared<CameraParser>());
  assetDatabase()->registerComponentParser(std::make_shared<PrefabParser>());

  // We're only creating this outside of the thread for the print() method.
  // Otherwise it would be in the thread.
  appState->root = RootGameObject::create(Guid{}, true /* lazyAttach */);

  // Startup the game by loading the initial scene.
  appState->root->addComponent(std::make_shared<InitialSceneLoadComponent>());
}

bool initializeBgfx(SDL_Window* window) {
  bgfx::PlatformData pd;
#if defined(SDL_PLATFORM_WIN32)
  pd.nwh = SDL_GetPointerProperty(
      SDL_GetWindowProperties(window),
      SDL_PROP_WINDOW_WIN32_HWND_POINTER,
      NULL);
  pd.ndt = NULL;
#elif defined(SDL_PLATFORM_MACOS)
  pd.nwh = SDL_GetPointerProperty(
      SDL_GetWindowProperties(window),
      SDL_PROP_WINDOW_COCOA_WINDOW_POINTER,
      NULL);
  pd.ndt = NULL;
#else
#error Unsupported platform, figure out what goes here.
#endif

  pd.context = NULL;
  pd.backBuffer = NULL;
  pd.backBufferDS = NULL;
  bgfx::setPlatformData(pd);

  bgfx::Init init;
  init.type = bgfx::RendererType::Count;
  init.vendorId = BGFX_PCI_ID_NONE;
  init.platformData.nwh = pd.nwh;
  init.platformData.ndt = pd.ndt;
  init.resolution.width = windowStartWidth;
  init.resolution.height = windowStartHeight;

  // Ensure BGFX is single threaded
  bgfx::renderFrame();

  // init.resolution.reset = BGFX_RESET_VSYNC;
  if (!bgfx::init(init)) {
    return false;
  }

  // bgfx::reset( 1280, 720, BGFX_RESET_VSYNC );
  bgfx::setDebug(BGFX_DEBUG_TEXT);
  bgfx::setViewRect(
      0, 0, 0, uint16_t(windowStartWidth), uint16_t(windowStartHeight));
  bgfx::setViewClear(
      0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030FF, 1.0f, 0);

  return true;
}

SDL_AppResult SDL_Fail() {
  SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
  return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  if (not SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    return SDL_Fail();
  }

  // init TTF
  if (not TTF_Init()) {
    return SDL_Fail();
  }

  SDL_Window* window = SDL_CreateWindow(
      "SDL Minimal Sample",
      windowStartWidth,
      windowStartHeight,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if (not window) {
    return SDL_Fail();
  }

  if (!initializeBgfx(window)) {
    return SDL_Fail();
  }

  SDL_ShowWindow(window);

  auto appState = new AppState{
      .sdlWindow = window,
      .width = windowStartWidth,
      .height = windowStartHeight,
  };
  initializeEngine(appState);

  *appstate = appState;
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  auto* app = reinterpret_cast<AppState*>(appstate);
  if (app) {
    bgfx::shutdown();
    SDL_DestroyWindow(app->sdlWindow);
    delete app;
  }

  SDL_Quit();
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  auto* app = reinterpret_cast<AppState*>(appstate);

  switch (event->type) {
    case SDL_EVENT_QUIT:
      app->run = false;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      std::cout << "Resized to: " << event->window.data1 << " "
                << event->window.data2 << std::endl;
      app->width = event->window.data1;
      app->height = event->window.data2;
      bgfx::reset(event->window.data1, event->window.data2);
      break;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  constexpr float kSimTickTime = 1.0f / 60.0f;
  auto* app = reinterpret_cast<AppState*>(appstate);

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

  bgfx::setViewRect(0, 0, 0, app->width, app->height);

  bgfx::touch(0);
  bgfx::dbgTextClear();
  int line = 0;
  print(root, 0, line);
  bgfx::frame();

  return app->run ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}
