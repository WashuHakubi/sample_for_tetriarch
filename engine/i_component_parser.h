/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

namespace ewok {
struct IComponentParser {
  virtual ~IComponentParser() = default;

  virtual auto create() const -> ComponentPtr = 0;

  virtual auto name() const -> std::string = 0;

  virtual void parse(
      ComponentPtr const& comp,
      ryml::ConstNodeRef component,
      std::unordered_map<std::string, GameObjectPtr> const& pathToObject)
      const = 0;
};
} // namespace ewok
