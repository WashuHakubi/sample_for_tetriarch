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
#include <ng-log/logging.h>

struct BgfxApplication : ew::IApplication {
  const unsigned kDefaultClearColor = 0x303030ff;
  const unsigned kAltClearColor = 0x334433ff;

  void handle(ew::Msg const& msg) override {
    if (auto resize = std::get_if<ew::ResizeMsg>(&msg)) {
      bgfx::reset(resize->width, resize->height, BGFX_RESET_VSYNC);
      bgfx::setViewRect(0, 0, 0, resize->width, resize->height);
      return;
    } else if (auto key = std::get_if<ew::KeyMsg>(&msg)) {
      if (key->scancode == ew::Scancode::SCANCODE_LSHIFT) {
        bgfx::setViewClear(
            0,
            BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            key->down ? kAltClearColor : kDefaultClearColor,
            1.0f,
            0);
      }
    }
  }

  bool init(int argc, char** argv) override {
    window_ = ew::createWindow("game", 800, 600, ew::WindowFlags::Resizable);
    if (!window_) {
      LOG(FATAL) << "Failed to create window";
    }

    // Don't create a rendering thread.
    bgfx::renderFrame();

    bgfx::Init init;
    auto [ndt, nwh] = window_->getWindowDescriptors();
    init.platformData.ndt = ndt;
    init.platformData.nwh = nwh;

    bgfx::init(init);

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, kDefaultClearColor, 1.0f, 0);

    auto [width, height] = window_->getWindowSize();
    bgfx::reset(width, height, BGFX_RESET_VSYNC);
    bgfx::setViewRect(0, 0, 0, width, height);
    return true;
  }

  void update() override {
    constexpr auto kSimTickRate = 1.0 / 60.0;
    constexpr auto kMaxSimTime = 5 * kSimTickRate;

    time_.update();

    // Cap the number of times we'll update the sim per frame to ensure rendering and other events are handled.
    timeAccumulator_ = std::max(timeAccumulator_ + time_.simDeltaTime(), kMaxSimTime);

    while (timeAccumulator_ >= kSimTickRate) {
      // update sim

      // Remove sim tick time from the accumulator
      timeAccumulator_ -= kSimTickRate;
      continue;
    }

    // Ensure view 0 is touched with at least a dummy event
    bgfx::touch(0);

    // draw

    // present frame
    bgfx::frame();
  }

private:
  double timeAccumulator_{0};
  ew::Time time_;
  ew::WindowPtr window_;
};

ew::ApplicationPtr ew::createApplication() {
  return std::make_shared<BgfxApplication>();
}