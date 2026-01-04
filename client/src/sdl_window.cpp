/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "i_window.h"

#include <SDL3/SDL.h>
#include <ng-log/logging.h>

namespace ew {

struct SDLWindow final : IWindow {
  SDLWindow(SDL_Window* window) : window_(window) {}
  ~SDLWindow() override { SDL_DestroyWindow(window_); }

  auto getWindowDescriptors() const -> std::tuple<std::string_view, void*, void*> override {
    auto windowProps = SDL_GetWindowProperties(window_);
    std::string_view currentVideoDriver = SDL_GetCurrentVideoDriver();
    LOG(INFO) << "Video driver: " << currentVideoDriver;

#if SDL_PLATFORM_LINUX
    if (currentVideoDriver == "x11") {
      auto xdisplay = SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
      auto xwindow = SDL_GetNumberProperty(windowProps, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
      return {currentVideoDriver, xdisplay, reinterpret_cast<void*>(xwindow)};
    } else if (currentVideoDriver == "wayland") {
      auto display = SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
      auto surface = SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
      return {currentVideoDriver, display, surface};
    } else {
      LOG(FATAL) << "Unknown video driver: " << currentVideoDriver;
    }
#elif SDL_PLATFORM_MACOS
    auto window = SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    return {currentVideoDriver, nullptr, window};
#elif SDL_PLATFORM_WIN32
    auto window = SDL_GetPointerProperty(windowProps, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    return {currentVideoDriver, nullptr, window};
#endif
  }

  auto getWindowSize() const -> std::pair<int, int> override {
    int w, h;
    SDL_GetWindowSize(window_, &w, &h);
    return std::make_pair(w, h);
  }

  void setWindowSize(int w, int h) override { SDL_SetWindowSize(window_, w, h); }

  void setFullscreen(bool fs) override {
    SDL_SetWindowFullscreen(window_, fs);
    SDL_SyncWindow(window_);
  }

  void captureMouse(bool capture) override {
    SDL_SetWindowRelativeMouseMode(window_, capture);
    // SDL_SetWindowMouseGrab(capture ? SDL_TRUE : SDL_FALSE);
  }

 private:
  SDL_Window* window_;
};

WindowPtr createWindow(std::string const& name, int width, int height, WindowFlags flags) {
  int sdl_flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
  if (all_of(flags, WindowFlags::FullScreen)) {
    sdl_flags |= SDL_WINDOW_FULLSCREEN;
  }

  if (all_of(flags, WindowFlags::Resizable)) {
    sdl_flags |= SDL_WINDOW_RESIZABLE;
  }

  auto windowPtr = SDL_CreateWindow(name.c_str(), width, height, sdl_flags);
  if (!windowPtr) {
    LOG(FATAL) << "Failed to create window '" << name << "': " << SDL_GetError();
    return nullptr;
  }

  return std::make_unique<SDLWindow>(windowPtr);
}
} // namespace ew