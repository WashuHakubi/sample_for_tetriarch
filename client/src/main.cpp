/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <typeindex>
#include <unordered_set>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_platform.h>
#include <SDL3/SDL_render.h>

#include <glm/glm.hpp>
#include <ng-log/logging.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>

#include <shared/event.h>

struct AppState {
  SDL_Window* window{nullptr};

  ~AppState() { SDL_DestroyWindow(window); }
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

  // Don't create a rendering thread.
  bgfx::renderFrame();

  bgfx::Init init;
#if BX_PLATFORM_LINUX
  std::string_view currentVideoDriver = SDL_GetCurrentVideoDriver();
  LOG(INFO) << "Video driver: " << currentVideoDriver;

  auto windowProps = SDL_GetWindowProperties(state->window);
  if (currentVideoDriver == "x11") {
    auto xdisplay = SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    init.platformData.ndt = xdisplay;

    auto xwindow = SDL_GetNumberProperty(windowProps, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    init.platformData.nwh = (void*)xwindow;
  } else if (currentVideoDriver == "wayland") {
    auto display = SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
    auto surface = SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);

    init.platformData.ndt = display;
    init.platformData.nwh = surface;
  } else {
    LOG(FATAL) << "Unknown video driver: " << currentVideoDriver;
  }
#elif BX_PLATFORM_OSX
  auto window = SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
  init.platformData.nwh = window;
#elif BX_PLATFORM_WINDOWS
  auto window = SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
  init.platformData.nwh = window;
#endif

  init.type = bgfx::RendererType::Count;

  bgfx::init(init);

  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
  bgfx::setViewRect(0, 0, 0, 800, 600);

  *appstate = state.release();

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  // auto state = static_cast<AppState*>(appstate);

  bgfx::touch(0);

  bgfx::frame();

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
  bgfx::shutdown();

  auto state = static_cast<AppState*>(appstate);
  delete state;
}