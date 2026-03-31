/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/component.h>
#include <wut/entity.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/platform.h>

#include <ng-log/logging.h>

int main(int argc, char** argv) {
  FLAGS_logtostdout = true;
  nglog::InitializeLogging(argv[0]);

  auto result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  if (!result) {
    LOG(FATAL) << "Failed to initialize SDL" << SDL_GetError();
  }

  int width = 800;
  int height = 600;

  auto window = SDL_CreateWindow("wut", width, height, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE);
  if (!window) {
    LOG(FATAL) << "Failed to create SDL window" << SDL_GetError();
  }

  auto wndProps = SDL_GetWindowProperties(window);

  // Force single threaded rendering.
  bgfx::renderFrame();

  bgfx::Init init;
  init.type = bgfx::RendererType::Count; // Use default renderer for platform.
  init.resolution.width = width;
  init.resolution.height = height;
  init.resolution.reset = BGFX_RESET_VSYNC;

#if BX_PLATFORM_OSX
  init.platformData.nwh = SDL_GetPointerProperty(wndProps, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#elif BX_PLATFORM_LINUX
  if (std::string_view{SDL_GetCurrentVideoDriver()} == "x11") {
    init.platformData.ndt = SDL_GetPointerProperty(wndProps, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
    init.platformData.nwh =
        reinterpret_cast<void*>(SDL_GetNumberProperty(wndProps, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
  } else {
    // must be wayland
    init.platformData.ndt = SDL_GetPointerProperty(wndProps, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
    init.platformData.nwh = SDL_GetPointerProperty(wndProps, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
  }
#elif BX_PLATFORM_WINDOWS
  init.platformData.nwh = SDL_GetPointerProperty(wndProps, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#else
#error Unsupported platform
#endif

  bgfx::init(init);

  auto root = wut::Entity::createRoot();

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x6495EDFF, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, width, height);
    bgfx::touch(0); // Touch the frame buffer to ensure we clear it.

    root->update();
    root->postUpdate();

    bgfx::frame();
    bgfx::renderFrame();
  }

  SDL_DestroyWindow(window);
  SDL_Quit();
}
