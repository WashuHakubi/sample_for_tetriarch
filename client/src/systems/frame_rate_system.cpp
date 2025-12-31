/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "frame_rate_system.h"

#include "debug_cubes_rendering_system.h"

void FrameRateSystem::render(float dt) {
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