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
#include <vector>

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
} // namespace detail

class EcsSystems {
 public:
  template <class T>
  void addSystem(std::shared_ptr<T> const& system);

  void clear();

  void render(float dt) const;

  void update(float dt) const;

 private:
  std::vector<std::shared_ptr<void>> systems_;
  std::vector<std::function<void(float dt)>> updateSystems_;
  std::vector<std::function<void(float dt)>> renderSystems_;
};

template <class T>
void EcsSystems::addSystem(std::shared_ptr<T> const& system) {
  systems_.push_back(system);

  if constexpr (detail::HasUpdate<T>::value) {
    updateSystems_.push_back([system](float dt) { system->update(dt); });
  }

  if constexpr (detail::HasRender<T>::value) {
    renderSystems_.push_back([system](float dt) { system->render(dt); });
  }
}
} // namespace ew
