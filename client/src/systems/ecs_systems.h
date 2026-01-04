/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <vector>

#include "../i_application.h"

namespace ew {
namespace detail {
template <class C>
struct HasUpdate {
 private:
  // ReSharper disable once CppFunctionIsNotImplemented
  template <class T>
  static constexpr auto check(int) -> decltype(std::declval<T>().update(std::declval<float>()), std::true_type{});

  // ReSharper disable once CppFunctionIsNotImplemented
  template <class T>
  static constexpr auto check(...) -> std::false_type;

  using type = decltype(check<C>(0));

 public:
  static constexpr bool value = type::value;
};

template <class C>
struct HasRender {
 private:
  // ReSharper disable once CppFunctionIsNotImplemented
  template <class T>
  static auto check(int) -> decltype(std::declval<T>().render(std::declval<float>()), std::true_type{});

  // ReSharper disable once CppFunctionIsNotImplemented
  template <class T>
  static auto check(...) -> std::false_type;

  using type = decltype(check<C>(0));

 public:
  static constexpr bool value = type::value;
};

template <class C>
struct HasMsgHandler {
 private:
  // ReSharper disable once CppFunctionIsNotImplemented
  template <class T>
  static constexpr auto check(int)
      -> decltype(std::declval<T>().handleMessage(std::declval<GameThreadMsg const&>()), std::true_type{});

  // ReSharper disable once CppFunctionIsNotImplemented
  template <class T>
  static constexpr auto check(...) -> std::false_type;

  using type = decltype(check<C>(0));

 public:
  static constexpr bool value = type::value;
};
} // namespace detail

class EcsSystems {
 public:
  template <class T, class... TArgs>
  void addSystem(TArgs&&... args);

  void clear();

  void render(float dt) const;

  void update(float dt) const;

  void handleMessage(GameThreadMsg const& msg) const;

  template <class T>
  std::shared_ptr<T> get();

 private:
  std::unordered_map<std::type_index, std::shared_ptr<void>> typeToSystem_;
  std::vector<std::shared_ptr<void>> systems_;
  std::vector<std::function<void(float dt)>> updateSystems_;
  std::vector<std::function<void(float dt)>> renderSystems_;
  std::vector<std::function<void(GameThreadMsg const&)>> messageHandlers_;
};

template <class T, class... TArgs>
void EcsSystems::addSystem(TArgs&&... args) {
  auto system = std::make_shared<T>(std::forward<TArgs>(args)...);
  typeToSystem_.emplace(typeid(T), system);
  systems_.push_back(system);

  if constexpr (detail::HasUpdate<T>::value) {
    updateSystems_.push_back([system](float dt) { system->update(dt); });
  }

  if constexpr (detail::HasRender<T>::value) {
    renderSystems_.push_back([system](float dt) { system->render(dt); });
  }

  if constexpr (detail::HasMsgHandler<T>::value) {
    messageHandlers_.push_back([system](GameThreadMsg const& msg) { system->handleMessage(msg); });
  }
}

template <class T>
std::shared_ptr<T> EcsSystems::get() {
  if (auto it = typeToSystem_.find(typeid(T)); it != typeToSystem_.end()) {
    return std::static_pointer_cast<T>(it->second);
  }
  assert(false && "Failed to find system matching type");
}
} // namespace ew
