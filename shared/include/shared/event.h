/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <functional>

#include <shared/dispose_handle.h>

namespace ew {
/// @brief Event handler. Wraps up a set of callables, returning a DisposeHandle for each subscription.
/// @tparam ...TArgs
template <class... TArgs>
struct Event;

template <class... TArgs>
using EventPtr = std::shared_ptr<Event<TArgs...>>;

template <class... TArgs>
struct Event : std::enable_shared_from_this<Event<TArgs...>> {
 private:
  struct InternalOnly {};

 public:
  static EventPtr<TArgs...> create() { return std::make_shared<Event>(InternalOnly{}); }

  explicit Event(InternalOnly const&) {}

  DisposeHandle subscribe(std::function<void(TArgs...)> fn) {
    if (free_.empty()) {
      auto weakSelf = std::weak_ptr<Event>{this->shared_from_this()};
      auto idx = callbacks_.size();
      callbacks_.push_back(std::move(fn));
      return DisposeHandle([weakSelf, idx] {
        auto p = weakSelf.lock();
        if (p) {
          p->unsubscribe(idx);
        }
      });
    }

    auto idx = free_.back();
    free_.pop_back();
    callbacks_[idx] = std::move(fn);
    return DisposeHandle([this, idx] { unsubscribe(idx); });
  }

  void invoke(TArgs... args) {
    for (auto&& cb : callbacks_) {
      if (cb) {
        cb(std::forward<TArgs>(args)...);
      }
    }
  }

 private:
  void unsubscribe(size_t idx) {
    callbacks_[idx] = {};
    free_.push_back(idx);
  }

  std::vector<std::function<void(TArgs...)>> callbacks_;
  std::vector<size_t> free_;
};
} // namespace ew