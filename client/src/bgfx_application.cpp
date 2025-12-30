/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <functional>
#include <iostream>

#include "i_application.h"
#include "i_window.h"

#include "time.h"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>
#include <bx/spscqueue.h>
#include <entt/entt.hpp>
#include <ng-log/logging.h>

namespace detail {
template <class C>
struct HasUpdate {
 private:
  template <typename T>
  static constexpr auto check(int) -> std::is_same<decltype(std::declval<T>().update(0.0f)), void>::type;

  template <typename>
  static constexpr auto check(...) -> std::false_type;

  using type = decltype(check<C>(0));

 public:
  static constexpr bool value = type::value;
};

template <class C>
struct HasRender {
 private:
  template <typename T>
  static constexpr auto check(int)
      -> std::is_same<decltype(std::declval<T>().render(std::declval<float>())), void>::type;

  template <typename>
  static constexpr auto check(...) -> std::false_type;

  using type = decltype(check<C>(0));

 public:
  static constexpr bool value = type::value;
};
} // namespace detail

template <class T>
struct EcsSystemFactory {
  static std::shared_ptr<T> create(entt::registry& r) {
    if constexpr (std::is_constructible_v<T, entt::registry&>) {
      return std::make_shared<T>(r);
    } else {
      return std::make_shared<T>();
    }
  }
};

static std::vector<std::function<void(entt::registry&)>> systemFactories_;
static std::vector<std::function<void()>> systemCleanup_;
static std::vector<std::function<void(float)>> systemUpdateFns_;
static std::vector<std::function<void(float)>> systemRenderFns_;

template <class T>
void registerSystem() {
  static std::shared_ptr<T> system;

  LOG(INFO) << "Registering system: " << typeid(T).name();
  systemFactories_.push_back([=](entt::registry& r) { system = EcsSystemFactory<T>::create(r); });

  systemCleanup_.push_back([=]() { system.reset(); });

  if constexpr (detail::HasUpdate<T>::value) {
    LOG(INFO) << "Registering system update: " << typeid(T).name() << " " << detail::HasUpdate<T>::value;
    systemUpdateFns_.push_back([](float dt) { system->update(dt); });
  }

  if constexpr (detail::HasRender<T>::value) {
    LOG(INFO) << "Registering system render: " << typeid(T).name() << " " << detail::HasRender<T>::value;
    systemRenderFns_.push_back([](float dt) { system->render(dt); });
  }
}

struct SimpleFrameRateSystem {
  void render(float dt) {
    bgfx::dbgTextClear();

    bgfx::dbgTextPrintf(0, 1, 0x0f, "\x1b[12;mFrame rate: %.1f", 1.0f / dt);
  }
};

struct BgfxApplication : ew::IApplication {
  const unsigned kDefaultClearColor = 0x303030ff;
  const unsigned kAltClearColor = 0x334433ff;

  BgfxApplication() : msgs_(&alloc_) {}

  void handle(ew::Msg const& msg) override {
    // Forward message to the game thread
    msgs_.push(new ew::Msg(msg));
  }

  bool init(int argc, char** argv) override {
    registerSystem<SimpleFrameRateSystem>();

    // Window is created on the main thread, which is also where rendering will happen
    window_ = ew::createWindow("game", 800, 600, ew::WindowFlags::Resizable);
    if (!window_) {
      LOG(FATAL) << "Failed to create window";
    }

    // Calling this before bgfx::init() ensures we don't create a rendering thread.
    bgfx::renderFrame();

    // These cannot be safely accessed on the game thread, so capture them here.
    auto descriptors = window_->getWindowDescriptors();

    // Send a message to the game thread containing the window dimensions.
    auto [width, height] = window_->getWindowSize();
    handle(ew::ResizeMsg{width, height});

    // Create the game thread.
    gameThread_ = std::thread([this, descriptors]() { this->run(descriptors); });

    return true;
  }

  bool update() override {
    // Render all the queued commands
    bgfx::renderFrame();

    if (exit_) {
      // Wait for the game thread to finish before we terminate.
      while (bgfx::renderFrame() != bgfx::RenderFrame::NoContext) {
      }
      gameThread_.join();
    }

    return !exit_;
  }

 private:
  void processMessages() {
    while (auto ptr = msgs_.pop(0)) {
      // Ensure the message always gets deleted.
      std::unique_ptr<ew::Msg> msg(ptr);

      if (auto resize = std::get_if<ew::ResizeMsg>(msg.get())) {
        bgfx::reset(resize->width, resize->height, BGFX_RESET_VSYNC);
        bgfx::setViewRect(0, 0, 0, resize->width, resize->height);
      } else if (auto key = std::get_if<ew::KeyMsg>(msg.get())) {
        if (key->scancode == ew::Scancode::SCANCODE_LSHIFT) {
          bgfx::setViewClear(
              0,
              BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
              key->down ? kAltClearColor : kDefaultClearColor,
              1.0f,
              0);
        }
      } else if (std::get_if<ew::ShutdownMsg>(msg.get())) {
        exit_ = true;
      }
    }
  }

  void run(std::pair<void*, void*> descriptors) {
    bgfx::Init init;

    auto [ndt, nwh] = descriptors;

    init.platformData.ndt = ndt;
    init.platformData.nwh = nwh;

    // This must be called on the "game" thread.
    bgfx::init(init);

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, kDefaultClearColor, 1.0f, 0);

    constexpr auto kSimTickRate = 1.0 / 60.0;
    constexpr auto kMaxSimTime = 5 * kSimTickRate;

    bgfx::setDebug(BGFX_DEBUG_TEXT);

    ew::Time time;
    double timeAccumulator{0};
    entt::registry registry;

    for (auto&& factory : systemFactories_) {
      factory(registry);
    }

    // Game loop
    while (!exit_) {
      processMessages();

      time.update();

      // Cap the number of times we'll update the sim per frame to ensure rendering and other events are handled.
      timeAccumulator = std::max(timeAccumulator + time.simDeltaTime(), kMaxSimTime);

      while (timeAccumulator >= kSimTickRate) {
        // update sim
        for (auto&& updateFn : systemUpdateFns_) {
          updateFn(kSimTickRate);
        }

        // Remove sim tick time from the accumulator
        timeAccumulator -= kSimTickRate;
      }

      // Ensure view 0 is touched with at least a dummy event
      bgfx::touch(0);

      for (auto&& renderFn : systemRenderFns_) {
        renderFn(kSimTickRate);
      }

      // present the frame, dispatches to the rendering thread.
      bgfx::frame();
    }

    for (auto&& cleanup : systemCleanup_) {
      cleanup();
    }

    bgfx::shutdown();
  }

  ew::WindowPtr window_;
  std::thread gameThread_;
  bx::DefaultAllocator alloc_;
  bx::SpScBlockingUnboundedQueueT<ew::Msg> msgs_;
  std::atomic_bool exit_{false};
};

ew::ApplicationPtr ew::createApplication() {
  return std::make_shared<BgfxApplication>();
}
