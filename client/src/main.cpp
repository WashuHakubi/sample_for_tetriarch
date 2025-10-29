/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>

#include <ng-log/logging.h>

struct AppState {
  SDL_Window* window{nullptr};
  SDL_Renderer* renderer{nullptr};
  SDL_GPUDevice* device{nullptr};
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
  FLAGS_logtostderr = 1;
  nglog::InitializeLogging(argv[0]);

  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    LOG(FATAL) << "Failed to initialize SDL: " << SDL_GetError();
  }

  auto state = std::make_unique<AppState>();

  state->window = SDL_CreateWindow("Test", 800, 600, 0);
  if (!state->window) {
    LOG(FATAL) << "Failed to create window: " << SDL_GetError();
  }

  state->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, nullptr);
  if (!state->device) {
    LOG(FATAL) << "Failed to create GPU device: " << SDL_GetError();
  }

  state->renderer = SDL_CreateGPURenderer(state->device, state->window);

  *appstate = state.release();
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  auto state = static_cast<AppState*>(appstate);

  SDL_RenderClear(state->renderer);

  SDL_RenderPresent(state->renderer);
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  // auto state = static_cast<AppState*>(appstate);
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  auto state = static_cast<AppState*>(appstate);
  SDL_DestroyRenderer(state->renderer);
  SDL_DestroyGPUDevice(state->device);
  SDL_DestroyWindow(state->window);
  delete state;
}