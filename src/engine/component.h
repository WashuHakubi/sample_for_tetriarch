/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"

namespace ewok {
class ComponentBase {
 public:
  virtual ~ComponentBase() = default;

  // Gets the game object that owns this component, or null if this component is
  // not owned.
  auto object() const noexcept -> GameObjectPtr { return parent_.lock(); }

  // called when a component is added to a game object
  virtual void attach() {}

  // called when a component is removed from a game object
  virtual void detach() {}

  // Run after each update() call. Useful for doing work that needs to happen
  // after any changes in update() have been applied
  virtual void postUpdate() {}

  // Run once per tick to render the object, only if the object is registered
  // for notifications
  virtual void render(Renderer& renderer, float dt) {}

  // Called once per frame to render UI.
  virtual void renderUI() {}

  // Run once per frame or simulation tick, whatever.
  virtual void update(float dt) {}

  virtual auto getComponentType() const -> std::type_index = 0;

 private:
  template <class T>
  friend class Component;

  friend class GameObject;

  // Require users to derive from Component<T>
  ComponentBase() = default;

  GameObjectHandle parent_;

  enum OverrideFlags {
    HasUpdate = 0x01,
    HasPostUpdate = 0x02,
    HasRender = 0x04,
  };

  // Defaults to all flags on.
  virtual auto getOverrideFlags() const -> OverrideFlags {
    return static_cast<OverrideFlags>(~0);
  }

  // If true then we assume update is overriden. Default to true in case the
  // user does not derive from Component<T>
  constexpr bool hasUpdate() const {
    return getOverrideFlags() & OverrideFlags::HasUpdate;
  }

  // If true then we assume postUpdate is overriden. Default to true in case the
  // user does not derive from Component<T>
  constexpr bool hasPostUpdate() const {
    return getOverrideFlags() & OverrideFlags::HasPostUpdate;
  }

  // If true then we assume postUpdate is overriden. Default to false in case
  // the user does not derive from Component<T>
  constexpr bool hasRender() const {
    return getOverrideFlags() & OverrideFlags::HasRender;
  }
};

template <class TDerived>
class Component : public ComponentBase {
 public:
  Component() {}

  auto getComponentType() const -> std::type_index override {
    return typeid(TDerived);
  }

 private:
  auto getOverrideFlags() const -> OverrideFlags final {
    return static_cast<OverrideFlags>(
        (std::is_same_v<
             decltype(&TDerived::update),
             decltype(&ComponentBase::update)>
             ? 0
             : OverrideFlags::HasUpdate) |
        (std::is_same_v<
             decltype(&TDerived::postUpdate),
             decltype(&ComponentBase::postUpdate)>
             ? 0
             : OverrideFlags::HasPostUpdate) |
        (std::is_same_v<
             decltype(&TDerived::render),
             decltype(&ComponentBase::render)>
             ? 0
             : OverrideFlags::HasRender));
  }
};

template <class TDerived>
class AsyncComponent : public Component<TDerived> {
 public:
  void attach() final { performAttachAsync(); }

  virtual concurrencpp::result<void> attachAsync() { co_return; }

 private:
  concurrencpp::null_result performAttachAsync() {
    try {
      co_await concurrencpp::resume_on(globalExecutor());
      co_await attachAsync();
    } catch (std::exception const& ex) {
      std::cerr << "Failed during attach: " << ex.what() << std::endl;
    } catch (...) {
      std::cout << "Failed during attach with unknown exception." << std::endl;
    }
  }
};
} // namespace ewok
