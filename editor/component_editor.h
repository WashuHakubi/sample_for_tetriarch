/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "engine/forward.h"
#include "engine/reflection/reflection.h"

namespace ewok {
struct ComponentEditor {
  virtual ~ComponentEditor() = default;

  virtual void draw(ComponentPtr const& component);
};

using DrawFn = void (*)(void* instance, std::unique_ptr<Field> const& field);
DrawFn getFieldDrawer(std::unique_ptr<Field> const& field);

void drawCompositeType(void* p, Class const* class_);
} // namespace ewok
