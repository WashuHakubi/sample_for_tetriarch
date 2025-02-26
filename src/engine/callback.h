/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"

namespace ewok {
template <class... TParams>
struct TCallback {
  size_t add(std::function<void(TParams...)> cb) {
    if (free_.empty()) {
      callbacks_.push_back(std::move(cb));
      return callbacks_.size() - 1;
    } else {
      auto idx = free_.back();
      free_.pop_back();
      callbacks_[idx] = std::move(cb);
      return idx;
    }
  }

  void clear() {
    free_.clear();
    callbacks_.clear();
  }

  void invoke(TParams&&... params) {
    for (auto&& callback : callbacks_) {
      if (callback) {
        callback(std::forward<TParams>(params)...);
      }
    }
  }

  void remove(size_t idx) {
    assert(idx < callbacks_.size());
    free_.push_back(idx);
    callbacks_[idx] = {};
  }

 private:
  std::vector<std::function<void(TParams...)>> callbacks_;
  std::vector<size_t> free_;
};

struct Callback : TCallback<GameObjectPtr const&> {};
} // namespace ewok
