/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>

#include "i_application.h"
#include "i_window.h"

#include "time.h"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>
#include <bx/spscqueue.h>
#include <ng-log/logging.h>

struct BgfxApplication : ew::IApplication {
  const unsigned kDefaultClearColor = 0x303030ff;
  const unsigned kAltClearColor = 0x334433ff;

  BgfxApplication() : msgs_(&alloc_) {}

  ~BgfxApplication() { gameThread_.join(); }

  void handle(ew::Msg const& msg) override { msgs_.push(new ew::Msg(msg)); }

  bool init(int argc, char** argv) override {
    window_ = ew::createWindow("game", 800, 600, ew::WindowFlags::Resizable);
    if (!window_) {
      LOG(FATAL) << "Failed to create window";
    }

    // Don't create a rendering thread.
    bgfx::renderFrame();

    // These cannot be safely accessed on the game thread, so capture them here.
    auto size = window_->getWindowSize();
    auto descriptors = window_->getWindowDescriptors();

    gameThread_ = std::thread([this, descriptors, size]() { this->run(descriptors, size); });

    return true;
  }

  bool update() override {
    // Render all the queued commands
    bgfx::renderFrame();

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

  void run(std::pair<void*, void*> descriptors, std::pair<int, int> size) {
    bgfx::Init init;

    auto [ndt, nwh] = descriptors;
    auto [width, height] = size;

    init.platformData.ndt = ndt;
    init.platformData.nwh = nwh;
    init.resolution.height = height;
    init.resolution.width = width;
    init.resolution.reset = BGFX_RESET_VSYNC;

    // This must be called on the "game" thread.
    bgfx::init(init);

    bgfx::reset(width, height, BGFX_RESET_VSYNC);
    bgfx::setViewRect(0, 0, 0, width, height);

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, kDefaultClearColor, 1.0f, 0);

    constexpr auto kSimTickRate = 1.0 / 60.0;
    constexpr auto kMaxSimTime = 5 * kSimTickRate;

    ew::Time time;
    double timeAccumulator{0};

    // Game loop
    while (true) {
      processMessages();
      if (exit_) {
        break;
      }

      time.update();

      // Cap the number of times we'll update the sim per frame to ensure rendering and other events are handled.
      timeAccumulator = std::max(timeAccumulator + time.simDeltaTime(), kMaxSimTime);

      while (timeAccumulator >= kSimTickRate) {
        // update sim

        // Remove sim tick time from the accumulator
        timeAccumulator -= kSimTickRate;
        continue;
      }

      // Ensure view 0 is touched with at least a dummy event
      bgfx::touch(0);

      // draw

      // present frame, dispatches to the rendering thread.
      bgfx::frame();
    }
  }

  ew::WindowPtr window_;
  std::thread gameThread_;
  bx::DefaultAllocator alloc_;
  bx::SpScBlockingUnboundedQueueT<ew::Msg> msgs_;
  bool exit_{false};
};

ew::ApplicationPtr ew::createApplication() {
  return std::make_shared<BgfxApplication>();
}
