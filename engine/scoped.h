/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <functional>

namespace ewok {
class Scoped {
 public:
  Scoped(std::function<void()> scopeExit) : scopeExit_(std::move(scopeExit)) {}

  ~Scoped() { scopeExit_(); }

 private:
  std::function<void()> scopeExit_;
};
} // namespace ewok

#define SCOPED_CONCAT_(x, y) x##y
#define SCOPED_CONCAT(x, y) SCOPED_CONCAT_(x, y)

#define SCOPED(fn)                                     \
  ::ewok::Scoped SCOPED_CONCAT(scoped_, __COUNTER__) { \
    fn                                                 \
  }
