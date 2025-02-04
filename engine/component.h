/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"
#include "engine/reflection.h"

namespace ewok {
class ComponentBase {
 public:
  // Gets the game object that owns this component, or null if this component is
  // not owned.
  auto object() const noexcept -> GameObject* { return parent_; }

  // called when a component is added to a game object
  virtual void attach() {}

  // called when a component is removed from a game object
  virtual void detach() {}

  // Run after each update() call. Useful for doing work that needs to happen
  // after any changes in update() have been applied
  virtual void postUpdate() {}

  // Run once per tick to render the object, only if the object is registered
  // for notifications
  virtual void render(Renderer& renderer) {}

  // Run once per frame or simulation tick, whatever.
  virtual void update(float dt) {}

 private:
  template <class T>
  friend class Component;

  friend class GameObject;

  // Require users to derive from Component<T>
  ComponentBase() = default;

  GameObject* parent_{nullptr};

  // If true then we assume update is overriden. Default to true in case the
  // user does not derive from Component<T>
  virtual bool hasUpdate() const { return true; }

  // If true then we assume postUpdate is overriden. Default to true in case the
  // user does not derive from Component<T>
  virtual bool hasPostUpdate() const { return true; }

  // If true then we assume postUpdate is overriden. Default to false in case
  // the user does not derive from Component<T>
  virtual bool hasRender() const { return false; }
};

template <class TDerived>
class Component : public ComponentBase {
 public:
  Component() {}

  static auto typeName() -> std::string { return getTypeName<TDerived>(); }

 private:
  bool hasUpdate() const final {
    return !std::is_same_v<
        decltype(&TDerived::update),
        decltype(&ComponentBase::update)>;
  }

  bool hasPostUpdate() const final {
    return !std::is_same_v<
        decltype(&TDerived::postUpdate),
        decltype(&ComponentBase::postUpdate)>;
  }

  bool hasRender() const final {
    return !std::is_same_v<
        decltype(&TDerived::render),
        decltype(&ComponentBase::render)>;
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
