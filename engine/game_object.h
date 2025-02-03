/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"
#include "engine/transform.h"

namespace ewok {
class GameObject {
 public:
  explicit GameObject(bool lazyAttach = false) : lazyAttach_(lazyAttach) {}

  constexpr bool active() const noexcept { return active_; }

  // Adds a child game object to this game object, if called during update or
  // postUpdate the add will be queued
  void addChild(GameObjectPtr child);

  // Adds a component to this game object, if called during update or postUpdate
  // the add will be queued. If isAttached is false then attach() will not be
  // called on the component.
  void addComponent(ComponentPtr component);

  // Gets the child game objects
  constexpr auto children() const noexcept -> std::span<GameObjectPtr const> {
    return children_;
  }

  // Gets the components of this game object
  constexpr auto components() const noexcept -> std::span<ComponentPtr const> {
    return components_;
  }

  constexpr auto name() const noexcept -> std::string const& { return name_; }

  // Get the full path of the game object.
  auto path() const noexcept -> std::string;

  // Gets the parent of this game object or nullptr if there is no parent
  constexpr auto parent() const noexcept -> GameObject* { return parent_; }

  // This runs after this object and every child object and component has
  // updated.
  void postUpdate();

  // Removes the component after the end of update or postUpdate.
  void queueRemoveChild(GameObjectPtr const& child);

  // Removes the component after the end of update or postUpdate.
  void queueRemoveComponent(ComponentPtr const& component);

  // Some components may wish to control if their update method is called. This
  // allows them to add themselves to have update and postUpdate called.
  void registerForUpdate(ComponentPtr const& component);

  // Removes a child game object, if called during update or postUpdate the
  // remove will be queued.
  void removeChild(GameObjectPtr const& child);

  // Removes a component, if called during update or postUpdate the
  // remove will be queued.
  void removeComponent(ComponentPtr const& component);

  void setActive(bool active);
  void setName(std::string name) { name_ = std::move(name); }
  void setTransform(Transform transform) { transform_ = transform; }

  auto transform() const noexcept -> Transform const& { return transform_; }

  // Some components may wish to control if their update method is called. This
  // allows them to remove themselves from having update and postUpdate called.
  void unregisterForUpdate(ComponentPtr const& component);

  // Update this object's components and then every child object
  void update(float dt);

 protected:
  // Fires the attach() call on all components. Should be run after all game
  // objects and components have been loaded. Meant for use with asset load
  // process.
  void fireAttached();

 private:
  void applyPostUpdateActions();

 private:
  enum class UpdateState {
    Idle,
    Update,
  };

  GameObject* parent_{nullptr};
  std::vector<GameObjectPtr> children_;
  std::vector<ComponentPtr> components_;
  std::unordered_set<ComponentBase*> updateComponents_;
  std::unordered_set<ComponentBase*> postUpdateComponents_;
  std::vector<std::function<void(GameObject*)>> postUpdateActions_;
  std::string name_;
  Transform transform_;
  UpdateState updateState_{UpdateState::Idle};
  bool active_{true};
  bool pendingActive_{true};
  bool lazyAttach_{false};
};
} // namespace ewok
