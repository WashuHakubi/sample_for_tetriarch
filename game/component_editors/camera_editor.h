/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "editor/i_component_editor.h"
#include "game/component/camera.h"

namespace ewok {
// Fake camera, not a real one, just showing off some component stuff.
class CameraEditor : public IComponentEditor {
 public:
  static CameraEditor* instance();

  void draw(ComponentPtr const& component) override;
};

} // namespace ewok
