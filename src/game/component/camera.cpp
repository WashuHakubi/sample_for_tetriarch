/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "game/component/camera.h"

#include <iostream>

EWOK_REGISTRATION {
  using namespace ewok;

  Register::class_<Camera>("Camera")
      .field(&Camera::name_, "name")
      .field(&Camera::target_, "target")
      .field(&Camera::f_, "f")
      .field(&Camera::s32_, "s32")
      .field(&Camera::u64_, "u64")
      .field(&Camera::floats_, "floats")
      .field(&Camera::vecs_, "vecs")
      .field(&Camera::t_, "t")
      .field(&Camera::cameraType_, "cameraType_");

  Register::enum_<CameraType>("CameraType")
      .value(CameraType::FirstPerson, "FirstPerson")
      .value(CameraType::ThirdPerson, "ThirdPerson")
      .value(CameraType::Free, "Free");
}

namespace ewok {
void Camera::attach() {
  std::cout << "I'm the camera: " << name_;
  auto t = target();
  if (t) {
    std::cout << " and I target: '" << t->path() << "'";
  }
  std::cout << std::endl;

  for (auto i = 0; i < 10; ++i) {
    floats_.push_back(static_cast<float>(i));
    if (i % 2) {
      vecs_.push_back({i + 0.f, i + 1.f, i - 1.f});
    }
  }
}
} // namespace ewok
