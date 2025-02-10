/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "engine/i_component_parser.h"

namespace ewok {
struct PrefabParser : IComponentParser {
  auto create() const -> ComponentPtr override;

  auto name() const -> std::string override;

  void parse(
      ComponentPtr const& comp,
      ryml::ConstNodeRef componentNode,
      GameObjectPtr const& root) const override;
};
} // namespace ewok
