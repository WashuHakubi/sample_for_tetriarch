/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <functional>
#include <utility>

namespace ew {

/// @brief Handle that when it is destroyed calls a function (if it has not been moved)
struct DisposeHandle {
  explicit DisposeHandle(std::function<void()> fn) : fn_(std::move(fn)) {}

  ~DisposeHandle() {
    if (fn_) {
      fn_();
    }
  }

  // not copyable
  DisposeHandle(DisposeHandle const&) = delete;
  DisposeHandle& operator=(DisposeHandle const&) = delete;

  DisposeHandle(DisposeHandle&& other) noexcept : fn_(std::exchange(other.fn_, nullptr)) {};

  DisposeHandle& operator=(DisposeHandle&& other) noexcept {
    if (std::addressof(other) == this) {
      return *this;
    }

    fn_ = std::move(other.fn_);
    other.fn_ = nullptr;
    return *this;
  };

 private:
  std::function<void()> fn_;
};
} // namespace ew