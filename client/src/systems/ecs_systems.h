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

#include "../assets/i_asset_provider.h"
#include "../i_application.h"

namespace ew {
namespace detail {
template <class C>
/// Check for an update(float) method.
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

/// Check for a render(float) method.
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

/// Check for a handleMessage(GameThreadMsg const&) method.
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
  struct InternalOnly {};

 public:
  static std::shared_ptr<EcsSystems> create(IApplicationPtr const& app, IAssetProviderPtr const& assetProvider);

  EcsSystems(InternalOnly const&) {}

  template <class T, class... TArgs>
  auto addSystem(TArgs&&... args) -> std::shared_ptr<T>;

  void clear();

  void render(float dt) const;

  void update(float dt) const;

  void handleMessage(GameThreadMsg const& msg) const;

  template <class T>
  [[nodiscard]] std::shared_ptr<T> get() const;

 private:
  std::unordered_map<std::type_index, std::shared_ptr<void>> typeToSystem_;
  std::vector<std::shared_ptr<void>> systems_;
  std::vector<std::function<void(float dt)>> updateSystems_;
  std::vector<std::function<void(float dt)>> renderSystems_;
  std::vector<std::function<void(GameThreadMsg const&)>> messageHandlers_;
};
using EcsSystemsPtr = std::shared_ptr<EcsSystems>;

template <class T, class... TArgs>
auto EcsSystems::addSystem(TArgs&&... args) -> std::shared_ptr<T> {
  auto system = std::make_shared<T>(std::forward<TArgs>(args)...);
  typeToSystem_.emplace(typeid(T), system);
  systems_.push_back(system);

  // Perform some checks to see if we have compatible methods and register them if we do. This is effectively a form of
  // static polymorphism. We don't register types that don't have the appropriate method signatures, so we're not
  // calling update on systems that don't need it. This could be replaced with an interface and virtual methods for very
  // little cost, since you shouldn't have more than a few hundred systems probably. However, since we can store
  // arbitrary objects in the EcsSystems container (such as entt::registry) we use this method instead.

  // Check if we have a compatible update method.
  if constexpr (detail::HasUpdate<T>::value) {
    updateSystems_.push_back([system](float dt) { system->update(dt); });
  }

  // Check if we have a compatible render method.
  if constexpr (detail::HasRender<T>::value) {
    renderSystems_.push_back([system](float dt) { system->render(dt); });
  }

  // Check if we have a compatible message handler method.
  if constexpr (detail::HasMsgHandler<T>::value) {
    messageHandlers_.push_back([system](GameThreadMsg const& msg) { system->handleMessage(msg); });
  }

  return system;
}

template <class T>
std::shared_ptr<T> EcsSystems::get() const {
  if (auto it = typeToSystem_.find(typeid(T)); it != typeToSystem_.end()) {
    return std::static_pointer_cast<T>(it->second);
  }
  assert(false && "Failed to find system matching type");
}
} // namespace ew
