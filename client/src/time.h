/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cassert>
#include <chrono>
namespace ew {
struct Time {
  using seconds_d = std::chrono::duration<double>;

  Time();

  void update();

  constexpr auto deltaTime() const { return deltaTime_.count(); }

  constexpr auto simDeltaTime() const { return simDeltaTime_.count(); }

  constexpr auto simTime() const { return simTime_; }

  constexpr auto timeScale() const { return timeScale_; }

  constexpr void timeScale(double v) {
    assert(v >= 0);
    timeScale_ = v;
  }

 private:
  std::chrono::high_resolution_clock::time_point prevTime_;
  std::chrono::nanoseconds simTime_{};

  double timeScale_{1};
  seconds_d deltaTime_{};
  seconds_d simDeltaTime_{};
};
} // namespace ew
