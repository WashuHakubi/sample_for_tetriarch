/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

namespace wut {
/**
 * Runs the passed lambda when this class goes out of scope.
 */
template <class Fn>
struct Scoped {
  Scoped(Fn fn) : fn_(std::forward<Fn>(fn)) {}

  ~Scoped() { fn_(); }

 private:
  Fn fn_;
};
} // namespace wut
