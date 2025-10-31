/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <ranges>
#include <typeindex>
#include <unordered_set>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>

#include <SDL3_shadercross/SDL_shadercross.h>

#include <ng-log/logging.h>

#include "entity_db.h"

struct AppState {
  SDL_Window* window{nullptr};
  SDL_Renderer* renderer{nullptr};
  SDL_GPUDevice* device{nullptr};
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
  FLAGS_logtostderr = true;
  nglog::InitializeLogging(argv[0]);

  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    LOG(FATAL) << "Failed to initialize SDL: " << SDL_GetError();
  }

  auto state = std::make_unique<AppState>();

  state->window = SDL_CreateWindow("Test", 800, 600, SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if (!state->window) {
    LOG(FATAL) << "Failed to create window: " << SDL_GetError();
  }

  state->device = SDL_CreateGPUDevice(
      SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_DXBC |
          SDL_GPU_SHADERFORMAT_DXIL,
      false,
      nullptr);
  if (!state->device) {
    LOG(FATAL) << "Failed to create GPU device: " << SDL_GetError();
  }

  state->renderer = SDL_CreateGPURenderer(state->device, state->window);

  *appstate = state.release();

  std::vector<ew::entity> entities;
  ew::entity_db db;
  for (uint32_t i = 0; i < 10; ++i) {
    auto ent = db.create();
    entities.push_back(ent);
    db.assign(ent, i);
    if (i % 2 == 0) {
      db.assign(ent, static_cast<float>(i));
    }
  }

  for (uint32_t i = 0; i < 10; i += 3) {
    db.destroy(entities[i]);
  }

  db.query<uint32_t>().visit([](ew::entity e, const uint32_t& i) { std::cout << static_cast<int>(e) << " = " << i << std::endl; });

  db.query<uint32_t, float>().visit([](ew::entity e, const uint32_t& i, float& f) { f += i + 2; });

  db.query<uint32_t, float>().visit([](ew::entity e, const uint32_t& i, float const& f) {
    std::cout << static_cast<int>(e) << " = " << i << ": " << f << std::endl;
  });

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