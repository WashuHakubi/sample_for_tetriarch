/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <functional>
#include <iostream>
#include <typeindex>

#include "i_application.h"
#include "i_window.h"

#include "ecs_systems.h"
#include "sim_time.h"

#include <utility>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>
#include <bx/math.h>
#include <bx/spscqueue.h>
#include <entt/entt.hpp>
#include <glm/ext.hpp>
#include <ng-log/logging.h>
#include <nlohmann/json.hpp>

#include "camera_system.h"
#include "debug_cubes_rendering_system.h"
#include "i_asset.h"
#include "simple_file_provider.h"

struct SimpleFrameRateSystem {
  void render(float dt) {
    accum_ += dt;
    ++count_;

    if (accum_ >= 0.5f) {
      // recomputes the frame rate every half-second
      rate_ = count_ / accum_;
      count_ = 0;
      accum_ = 0;
    }

    bgfx::dbgTextPrintf(0, 1, 0x0b, "Frame rate: %.1f", rate_);
  }

  float rate_{0};
  float count_{0};
  float accum_{0};
};

struct SetFullScreenMsg {
  bool fullscreen;
};

using MainMsg = std::variant<SetFullScreenMsg>;

struct BgfxApplication : ew::IApplication {
  const unsigned kDefaultClearColor = 0x303030ff;
  const unsigned kAltClearColor = 0x334433ff;

  BgfxApplication();

  void handle(ew::Msg const& msg) override;

  bool init(int argc, char** argv) override;

  bool update() override;

 private:
  void processMessages(ew::SimTime const& time);

  void run(std::pair<void*, void*> descriptors);

  ew::WindowPtr window_;
  std::thread gameThread_;
  bx::DefaultAllocator alloc_;
  bx::SpScBlockingUnboundedQueueT<ew::Msg> gameMsgs_;
  bx::SpScBlockingUnboundedQueueT<MainMsg> mainMsgs_;
  ew::EcsSystems systems_;
  std::atomic_bool exit_{false};
  bool fullscreen_{false};
  std::pair<int, int> windowSize_{800, 600};
};

BgfxApplication::BgfxApplication() : gameMsgs_(&alloc_), mainMsgs_(&alloc_) {}

void BgfxApplication::handle(ew::Msg const& msg) {
  // Forward message to the game thread
  gameMsgs_.push(new ew::Msg(msg));
}

bool BgfxApplication::init(int argc, char** argv) {
  // Window is created on the main thread, which is also where rendering will happen
  window_ = ew::createWindow("game", windowSize_.first, windowSize_.second, ew::WindowFlags::Resizable);
  if (!window_) {
    LOG(FATAL) << "Failed to create window";
  }

  // Calling this before bgfx::init() ensures we don't create a rendering thread.
  bgfx::renderFrame();

  // These cannot be safely accessed on the game thread, so capture them here.
  auto descriptors = window_->getWindowDescriptors();

  // Send a message to the game thread containing the window dimensions.
  windowSize_ = window_->getWindowSize();
  auto [width, height] = windowSize_;
  handle(ew::ResizeMsg{width, height});

  // Create the game thread.
  gameThread_ = std::thread([this, descriptors]() { this->run(descriptors); });

  return true;
}

bool BgfxApplication::update() {
  // Render all the queued commands
  bgfx::renderFrame();

  // Process any requests from the game thread.
  while (auto const ptr = mainMsgs_.pop(0)) {
    std::unique_ptr<MainMsg> msg(ptr);

    if (auto const fs = std::get_if<SetFullScreenMsg>(msg.get())) {
      window_->setFullscreen(fs->fullscreen);
    }
  }

  if (exit_) {
    // Wait for the game thread to finish before we terminate.
    while (bgfx::renderFrame() != bgfx::RenderFrame::NoContext) {
    }
    gameThread_.join();
  }

  return !exit_;
}

void BgfxApplication::processMessages(ew::SimTime const& time) {
  while (auto const ptr = gameMsgs_.pop(0)) {
    // Ensure the message always gets deleted.
    std::unique_ptr<ew::Msg> msg(ptr);

    systems_.handleMessage(*msg);

    if (auto const resize = std::get_if<ew::ResizeMsg>(msg.get())) {
      windowSize_ = {resize->width, resize->height};
      bgfx::reset(resize->width, resize->height, BGFX_RESET_VSYNC);
      bgfx::setViewRect(0, 0, 0, resize->width, resize->height);
    } else if (auto const key = std::get_if<ew::KeyMsg>(msg.get())) {
      if (key->scancode == ew::Scancode::SCANCODE_LSHIFT) {
        bgfx::setViewClear(
            0,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            key->down ? kAltClearColor : kDefaultClearColor,
            1.0f,
            0);
      }

      if (key->scancode == ew::Scancode::SCANCODE_F1 && key->down) {
        fullscreen_ = !fullscreen_;

        // Request the render thread to switch our full-screen status.
        mainMsgs_.push(new MainMsg{SetFullScreenMsg{fullscreen_}});
      }
    } else if (std::get_if<ew::ShutdownMsg>(msg.get())) {
      exit_ = true;
    }
  }
}

void BgfxApplication::run(std::pair<void*, void*> descriptors) {
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

  ew::SimTime time;
  double timeAccumulator{0};
  entt::registry registry;

  auto assetProvider = std::make_shared<ew::AssetProvider>(std::make_shared<SimpleFileProvider>("assets"));
  assetProvider->registerAssetLoader(std::make_shared<ShaderProgramLoader>());

  systems_.addSystem(std::make_shared<SimpleFrameRateSystem>());
  systems_.addSystem(std::make_shared<ew::CameraSystem>(registry));
  systems_.addSystem(std::make_shared<DebugCubesRenderingSystem>(assetProvider));

  // Game loop
  while (!exit_) {
    time.update();

    processMessages(time);

    // Cap the number of times we'll update the sim per frame to ensure rendering and other events are handled.
    timeAccumulator = std::max(timeAccumulator + time.simDeltaTime(), kMaxSimTime);

    while (timeAccumulator >= kSimTickRate) {
      // update sim
      systems_.update(kSimTickRate);

      // Remove sim tick time from the accumulator
      timeAccumulator -= kSimTickRate;
    }

    // Ensure view 0 is touched with at least a dummy event
    bgfx::touch(0);

    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 0, 0x0b, "Dimensions: %d x %d", windowSize_.first, windowSize_.second);

    systems_.render(static_cast<float>(time.simDeltaTime()));

    // present the frame, dispatches to the rendering thread.
    bgfx::frame();
  }

  systems_.clear();

  bgfx::shutdown();
}

ew::ApplicationPtr ew::createApplication() {
  return std::make_shared<BgfxApplication>();
}
