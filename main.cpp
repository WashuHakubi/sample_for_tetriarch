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

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"
#include "imgui.h"

#include <filesystem>
#include <format>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

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
  SDL_GPUDevice* gpuDevice;

  std::shared_ptr<Renderer> renderer;
  std::shared_ptr<RootGameObject> root;

  bool run{true};
  bool showDemoWindow{true};
  uint64_t prevTime{};
};

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

  // create a renderer
  SDL_GPUDevice* gpuDevice = SDL_CreateGPUDevice(
      SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL |
          SDL_GPU_SHADERFORMAT_METALLIB,
      true,
      nullptr);
  if (gpuDevice == nullptr) {
    return SDL_Fail();
  }

  if (!SDL_ClaimWindowForGPUDevice(gpuDevice, window)) {
    return SDL_Fail();
  }

  SDL_SetGPUSwapchainParameters(
      gpuDevice,
      window,
      SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
      SDL_GPU_PRESENTMODE_MAILBOX);

  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

  ImGui::StyleColorsDark();
  ImGui_ImplSDL3_InitForSDLGPU(window);
  ImGui_ImplSDLGPU3_InitInfo init_info = {};
  init_info.Device = gpuDevice;
  init_info.ColorTargetFormat =
      SDL_GetGPUSwapchainTextureFormat(gpuDevice, window);
  init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
  ImGui_ImplSDLGPU3_Init(&init_info);

  auto appState = new AppState{
      .sdlWindow = window,
      .gpuDevice = gpuDevice,
  };
  initializeEngine(appState);

  *appstate = appState;
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  auto* app = reinterpret_cast<AppState*>(appstate);
  if (app) {
    SDL_WaitForGPUIdle(app->gpuDevice);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(app->gpuDevice, app->sdlWindow);
    SDL_DestroyGPUDevice(app->gpuDevice);
    SDL_DestroyWindow(app->sdlWindow);
    delete app;
  }

  SDL_Quit();
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  auto* app = reinterpret_cast<AppState*>(appstate);

  if (event->type == SDL_EVENT_QUIT) {
    app->run = false;
  }

  ImGui_ImplSDL3_ProcessEvent(event);

  return SDL_APP_CONTINUE;
}

void RenderImgui(AppState* app) {
  ImGui_ImplSDLGPU3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  if (app->showDemoWindow) {
    ImGui::ShowDemoWindow(&app->showDemoWindow);
  }

  ImGui::Render();
  ImDrawData* drawData = ImGui::GetDrawData();
  SDL_GPUCommandBuffer* commandBuffer =
      SDL_AcquireGPUCommandBuffer(app->gpuDevice);

  SDL_GPUTexture* swapChainTexture;
  SDL_AcquireGPUSwapchainTexture(
      commandBuffer,
      app->sdlWindow,
      &swapChainTexture,
      nullptr,
      nullptr); // Acquire a swapchain texture

  if (swapChainTexture != nullptr) {
    // This is mandatory: call Imgui_ImplSDLGPU3_PrepareDrawData() to upload the
    // vertex/index buffer!
    Imgui_ImplSDLGPU3_PrepareDrawData(drawData, commandBuffer);

    // Setup and start a render pass
    SDL_GPUColorTargetInfo target_info = {};
    target_info.texture = swapChainTexture;
    target_info.clear_color = SDL_FColor{0.45f, 0.55f, 0.60f, 1.00f};
    target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    target_info.store_op = SDL_GPU_STOREOP_STORE;
    target_info.mip_level = 0;
    target_info.layer_or_depth_plane = 0;
    target_info.cycle = false;
    SDL_GPURenderPass* renderPass =
        SDL_BeginGPURenderPass(commandBuffer, &target_info, 1, nullptr);

    // Render ImGui
    ImGui_ImplSDLGPU3_RenderDrawData(drawData, commandBuffer, renderPass);

    SDL_EndGPURenderPass(renderPass);
  }

  // Submit the command buffer
  SDL_SubmitGPUCommandBuffer(commandBuffer);
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

  RenderImgui(app);

  return app->run ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}
