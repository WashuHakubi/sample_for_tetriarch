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

#include "editor/editor.h"

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

#include <SDL3_shadercross/SDL_shadercross.h>

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

struct PositionColorVertex {
  Vec3 position;
  UInt8Vec4 color;
};

struct AppState {
  concurrencpp::runtime runtime;
  std::shared_ptr<concurrencpp::manual_executor> executor;

  SDL_Window* sdlWindow;
  SDL_GPUDevice* gpuDevice;
  SDL_GPUGraphicsPipeline* sdlPipeline;
  SDL_GPUBuffer* vertexBuffer;

  std::shared_ptr<Renderer> renderer;
  std::shared_ptr<RootGameObject> root;
  std::shared_ptr<Editor> editor;

  bool run{true};
  bool showDemoWindow{true};
  bool showImguiUI{true};
  uint64_t prevTime{};
};

SDL_GPUShader* buildShader(
    SDL_GPUDevice* gpuDevice,
    std::string const& name,
    SDL_ShaderCross_ShaderStage stage) {
  // Compile a shader
  auto const& sfp = assetDatabase()->getFileProvider();
  auto source = sfp->blockingReadFile(name).value();

  SDL_ShaderCross_SPIRV_Info info = {
      .bytecode = reinterpret_cast<uint8_t const*>(source.data()),
      .bytecode_size = source.size(),
      .entrypoint = "main",
      .shader_stage = stage,
      .enable_debug = false,
      .name = nullptr,
      .props = 0,
  };

  SDL_ShaderCross_GraphicsShaderMetadata metadata;
  return SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
      gpuDevice, &info, &metadata);
}

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
      SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_METALLIB,
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
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
      ImGuiConfigFlags_DockingEnable; // Enable Keyboard Controls

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

  auto vertShader = buildShader(
      gpuDevice,
      "shaders/position_color.vert.spv",
      SDL_ShaderCross_ShaderStage::SDL_SHADERCROSS_SHADERSTAGE_VERTEX);
  assert(vertShader);

  auto fragShader = buildShader(
      gpuDevice,
      "shaders/solid_color.frag.spv",
      SDL_ShaderCross_ShaderStage::SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT);
  assert(fragShader);

  SDL_GPUColorTargetDescription colorTargetDescs[] = {
      {
          .format = SDL_GetGPUSwapchainTextureFormat(gpuDevice, window),
      },
  };

  // Create the pipeline
  SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .target_info =
          {
              .num_color_targets = 1,
              .color_target_descriptions = colorTargetDescs,
          },
      // This is set up to match the vertex shader layout!
      .vertex_input_state =
          (SDL_GPUVertexInputState){
              .num_vertex_buffers = 1,
              .vertex_buffer_descriptions =
                  (SDL_GPUVertexBufferDescription[]){
                      {.slot = 0,
                       .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                       .instance_step_rate = 0,
                       .pitch = sizeof(PositionColorVertex)}},
              .num_vertex_attributes = 2,
              .vertex_attributes =
                  (SDL_GPUVertexAttribute[]){
                      {.buffer_slot = 0,
                       .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                       .location = 0,
                       .offset = 0},
                      {.buffer_slot = 0,
                       .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                       .location = 1,
                       .offset = sizeof(float) * 3}}},
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .vertex_shader = vertShader,
      .fragment_shader = fragShader};

  appState->sdlPipeline =
      SDL_CreateGPUGraphicsPipeline(gpuDevice, &pipelineCreateInfo);
  if (!appState->sdlPipeline) {
    return SDL_Fail();
  }

  SDL_ReleaseGPUShader(gpuDevice, vertShader);
  SDL_ReleaseGPUShader(gpuDevice, fragShader);

  auto bufferCreateInfo = SDL_GPUBufferCreateInfo{
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
      .size = sizeof(PositionColorVertex) * 3,
  };

  // Create the vertex buffer
  appState->vertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &bufferCreateInfo);

  auto transferBufferCreateInfo = SDL_GPUTransferBufferCreateInfo{
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = sizeof(PositionColorVertex) * 3,
  };

  SDL_GPUTransferBuffer* transferBuffer =
      SDL_CreateGPUTransferBuffer(gpuDevice, &transferBufferCreateInfo);

  PositionColorVertex* transferData = reinterpret_cast<PositionColorVertex*>(
      SDL_MapGPUTransferBuffer(gpuDevice, transferBuffer, false));

  transferData[0] = (PositionColorVertex){-1, -1, 0, 255, 0, 0, 255};
  transferData[1] = (PositionColorVertex){1, -1, 0, 0, 255, 0, 255};
  transferData[2] = (PositionColorVertex){0, 1, 0, 0, 0, 255, 255};

  SDL_UnmapGPUTransferBuffer(gpuDevice, transferBuffer);

  // Upload the transfer data to the vertex buffer
  SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpuDevice);
  SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

  auto transferBufferLoc = SDL_GPUTransferBufferLocation{
      .transfer_buffer = transferBuffer,
      .offset = 0,
  };
  auto bufferRegion = SDL_GPUBufferRegion{
      .buffer = appState->vertexBuffer,
      .offset = 0,
      .size = sizeof(PositionColorVertex) * 3,
  };
  SDL_UploadToGPUBuffer(copyPass, &transferBufferLoc, &bufferRegion, false);

  SDL_EndGPUCopyPass(copyPass);
  SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
  SDL_ReleaseGPUTransferBuffer(gpuDevice, transferBuffer);

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

    SDL_ReleaseGPUGraphicsPipeline(app->gpuDevice, app->sdlPipeline);
    SDL_ReleaseGPUBuffer(app->gpuDevice, app->vertexBuffer);
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

void RenderImgui(
    AppState* app,
    SDL_GPUTexture* swapChainTexture,
    SDL_GPUCommandBuffer* commandBuffer) {
  if (!app->showImguiUI) {
    return;
  }

  ImGui_ImplSDLGPU3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::DockSpaceOverViewport(
      0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

  // Do your imgui rendering here...
  // if (app->showDemoWindow) {
  //   ImGui::ShowDemoWindow(&app->showDemoWindow);
  // }

  // draw returns true to exit. But we may have tried to close the window
  // so we merge run with the return of draw.
  app->run = app->run && !app->editor->draw(app->root);

  // Render any game object components.
  app->root->renderUI();

  ImGui::Render();

  // Draw the imgui UI to the screen. This is taken mostly from imgui sample
  // code.
  ImDrawData* drawData = ImGui::GetDrawData();

  // This is mandatory: call Imgui_ImplSDLGPU3_PrepareDrawData() to upload the
  // vertex/index buffer!
  Imgui_ImplSDLGPU3_PrepareDrawData(drawData, commandBuffer);
}

void drawTriangle(
    AppState* app,
    SDL_GPUTexture* swapchainTexture,
    SDL_GPUCommandBuffer* cmdbuf,
    SDL_GPURenderPass* renderPass) {
  SDL_GPUColorTargetInfo colorTargetInfo = {0};

  SDL_BindGPUGraphicsPipeline(renderPass, app->sdlPipeline);

  auto bufferBinding = SDL_GPUBufferBinding{
      .buffer = app->vertexBuffer,
      .offset = 0,
  };

  SDL_BindGPUVertexBuffers(renderPass, 0, &bufferBinding, 1);
  SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
  return;
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

  SDL_GPUTexture* swapChainTexture;

  SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(app->gpuDevice);
  SDL_WaitAndAcquireGPUSwapchainTexture(
      cmdbuf,
      app->sdlWindow,
      &swapChainTexture,
      nullptr,
      nullptr); // Acquire a swapchain texture

  RenderImgui(app, swapChainTexture, cmdbuf);

  if (swapChainTexture != nullptr) {
    SDL_GPUColorTargetInfo target_info = {};
    target_info.texture = swapChainTexture;
    target_info.clear_color = SDL_FColor{0, 0, 0, 0};
    target_info.load_op = SDL_GPU_LOADOP_LOAD;
    target_info.store_op = SDL_GPU_STOREOP_STORE;
    target_info.mip_level = 0;
    target_info.layer_or_depth_plane = 0;
    target_info.cycle = false;
    SDL_GPURenderPass* renderPass =
        SDL_BeginGPURenderPass(cmdbuf, &target_info, 1, nullptr);

    drawTriangle(app, swapChainTexture, cmdbuf, renderPass);

    ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), cmdbuf, renderPass);

    SDL_EndGPURenderPass(renderPass);
  }

  // Submit the command buffer
  SDL_SubmitGPUCommandBuffer(cmdbuf);

  return app->run ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}
