/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

namespace ewok::shared {
struct MessageDispatchAutoRelease {
  virtual ~MessageDispatchAutoRelease() = default;
};

using MsgDispatchHandle = std::unique_ptr<MessageDispatchAutoRelease>;

template <class TMsg>
struct MessageDispatch {
  static void send(TMsg const& msg) {
    for (auto&& handler : s_handlers) {
      if (handler) [[likely]] {
        handler(msg);
      }
    }
  }

  static auto subscribe(std::function<void(TMsg const&)> handler) -> MsgDispatchHandle {
    if (s_freeIds.empty()) {
      auto id = s_handlers.size();
      s_handlers.emplace_back(std::move(handler));
      return std::make_unique<HandleData>(id);
    }

    auto id = s_freeIds.back();
    s_freeIds.pop_back();
    s_handlers[id] = std::move(handler);
    return std::make_unique<HandleData>(id);
  }

private:
  struct HandleData : MessageDispatchAutoRelease {
    explicit HandleData(size_t id)
      : id_(id) {
    }

    ~HandleData() override { unsubscribe(id_); }

  private:
    friend struct MessageDispatch;

    size_t id_;
  };

  static void unsubscribe(size_t id) {
    assert(id < s_handlers.size());
    assert(s_handlers[id]);

    s_handlers[id] = nullptr;
    s_freeIds.emplace_back(id);
  }

  static std::vector<std::function<void(TMsg const&)>> s_handlers;
  static std::vector<size_t> s_freeIds;
};

template <class TMsg>
inline std::vector<std::function<void(TMsg const&)>> MessageDispatch<TMsg>::s_handlers;

template <class TMsg>
inline std::vector<size_t> MessageDispatch<TMsg>::s_freeIds;

namespace detail {
template <class T, class = void>
struct DispatchSig;

template <class TMsg>
struct DispatchSig<void(TMsg)> {
  using type = std::remove_cvref_t<TMsg>;
};

template <typename C, typename TMsg>
struct DispatchSig<void (C::*)(TMsg)> {
  using type = std::remove_cvref_t<TMsg>;
};

template <typename C, typename TMsg>
struct DispatchSig<void (C::*)(TMsg) const> {
  using type = std::remove_cvref_t<TMsg>;
};

// For lambdas, we need to reroute to the appropriate member function signature.
template <typename T>
struct DispatchSig<T, std::void_t<decltype(&T::operator())>> : DispatchSig<decltype(&T::operator())> {
};
} // namespace detail

template <class TLambda>
auto subscribeMessage(TLambda&& fn) {
  using TMsg = detail::DispatchSig<TLambda>::type;
  return MessageDispatch<TMsg>::subscribe(std::forward<TLambda>(fn));
}

template <class TMsg>
void sendMessage(TMsg const& msg) {
  MessageDispatch<TMsg>::send(msg);
}
} // namespace ewok::shared