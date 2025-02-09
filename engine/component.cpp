/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "engine/component.h"
#include "editor/component_editor.h"

namespace ewok {
static ComponentEditor s_defaultEditor;

auto ComponentBase::getComponentEditor() const -> ComponentEditor* {
  return &s_defaultEditor;
}
} // namespace ewok
