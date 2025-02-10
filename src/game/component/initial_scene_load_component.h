/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include "engine/component.h"

namespace ewok {
class InitialSceneLoadComponent
    : public AsyncComponent<InitialSceneLoadComponent>,
      public std::enable_shared_from_this<InitialSceneLoadComponent> {
  concurrencpp::result<void> attachAsync() override;
};
} // namespace ewok
