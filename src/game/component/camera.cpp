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

  Reflection::class_<Camera>("Camera")
      .field(&Camera::name_, "name")
      .field(&Camera::target_, "target")
      .field(&Camera::f_, "f")
      .field(&Camera::s32_, "s32")
      .field(&Camera::u64_, "u64");
}

namespace ewok {
void Camera::attach() {
  std::cout << "I'm the camera: " << name_;
  auto t = target();
  if (t) {
    std::cout << " and I target: '" << t->path() << "'";
  }
  std::cout << std::endl;
}
} // namespace ewok
