/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "i_application.h"

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
#include <bx/spscqueue.h>

#include <shared/event.h>

struct AppState {
  ew::ApplicationPtr app;
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
  FLAGS_logtostderr = true;
  nglog::InitializeLogging(argv[0]);

  if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
    LOG(FATAL) << "Failed to initialize SDL: " << SDL_GetError();
  }

  auto state = std::make_unique<AppState>();
  state->app = ew::createApplication();

  if (!state->app->init(argc, argv)) {
    return SDL_APP_FAILURE;
  }

  *appstate = state.release();

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  auto state = static_cast<AppState*>(appstate);

  return state->app->update() ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  auto state = static_cast<AppState*>(appstate);

  switch (event->type) {
    case SDL_EVENT_QUIT:
      state->app->handle(ew::ShutdownMsg{});
      return SDL_APP_CONTINUE;

    case SDL_EVENT_WINDOW_RESIZED: {
      auto width = event->window.data1;
      auto height = event->window.data2;

      state->app->handle(ew::ResizeMsg{width, height});
      break;
    }
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      state->app->handle(
          ew::KeyMsg{
              // Could map this to a custom enumeration, but I'm lazy atm.
              .scancode = static_cast<ew::Scancode>(event->key.scancode),
              .down = event->type == SDL_EVENT_KEY_DOWN,
          });
      break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      state->app->handle(
          ew::MouseButtonMsg{
              .button = event->button.button,
              .clicks = event->button.clicks,
              .down = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN,
          });
      break;

    case SDL_EVENT_MOUSE_MOTION:
      state->app->handle(
          ew::MouseMotionMsg{
              .absPosition = glm::vec2(event->motion.x, event->motion.y),
              .relPosition = glm::vec2(event->motion.xrel, event->motion.yrel),
          });
      break;

    case SDL_EVENT_MOUSE_WHEEL:
      state->app->handle(
          ew::MouseWheelMsg{
              .delta = event->wheel.y,
          });
      break;
    default:
      break;
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  auto state = static_cast<AppState*>(appstate);
  delete state;
}
