/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "sim_time.h"

ew::SimTime::SimTime() : prevTime_(std::chrono::high_resolution_clock::now()) {}

void ew::SimTime::update() {
  using namespace std::chrono;

  const auto curTime = high_resolution_clock::now();
  const nanoseconds delta = curTime - prevTime_;
  deltaTime_ = delta;

  simDeltaTime_ = deltaTime_ * timeScale_;
  simTime_ += duration_cast<nanoseconds>(simDeltaTime_);
  prevTime_ = curTime;
}
