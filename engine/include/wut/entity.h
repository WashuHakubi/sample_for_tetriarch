/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <bitset>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <wut/fwd.h>

namespace wut {
class Entity : public std::enable_shared_from_this<Entity> {
 public:
  static auto createRoot() -> EntityPtr;

 public:
  template <class T>
  auto component() const -> std::shared_ptr<T> {
    return std::static_pointer_cast<T>(component(typeid(T)));
  }

  /**
   * Gets a component matching type on this entity or nullptr if it does not exist. If multiple components match the
   * type then it is unspecified which will be returned.
   */
  auto component(std::type_index type) const -> ComponentPtr;

  /**
   * True if the entity is in the scene, the entity is enabled, and all ancestors are also enabled.
   */
  auto enabledInTree() const { return flags_.test(detail::FLAG_ENABLED_IN_TREE); }

  /**
   * True if the entity is enabled locally.
   */
  auto enabledSelf() const { return flags_.test(detail::FLAG_ENABLED); }

  /**
   * Returns a strong reference to the parent of this entity, or nullptr if the entity does not have a parent.
   */
  auto parent() const { return parent_ ? parent_->shared_from_this() : nullptr; }

  /**
   * Returns a strong reference to the root of the tree, or nullptr if this is not in the tree.
   */
  auto root() const { return root_.lock(); }

 public:
  /**
   * Adds a component to this entity. If this entity is enabled in the tree and the component is enabled then
   * onEnabled() will be fired.
   */
  void addComponent(ComponentPtr const& component);

  /**
   * Schedules this entity to be destroyed by the end of the next update.
   */
  void destroy();

  /**
   * Changes the local enabled state. If this changes the enabled in tree state then onEnabled or onDisabled will be
   * fired for all enabled components and child entities of this entity.
   */
  void setEnabled(bool enabled);

  /**
   * Sets the parent entity. If the parent is enabled in the tree this will result in onEnabled or onDisabled being
   * fired for all enabled components and child objects of this entity.
   *
   * This may only be called once, and parent may not be null.
   */
  void setParent(EntityPtr const& parent);

  /**
   * Fires the update event for all enabled components and children on this entity.
   */
  void update();

  /**
   * Fires the postUpdate event for all enabled components and children on this entity.
   */
  void postUpdate();

 private:
  void updateEnabledRecurse(bool newState);

 private:
  friend class Component;

  Entity* parent_{nullptr};
  EntityHandle root_{};

  std::vector<EntityPtr> children_;
  size_t childCount_{0};

  std::vector<ComponentPtr> components_;
  size_t componentCount_{0};

  std::unordered_map<std::type_index, ComponentPtr> typeToComponents_;
  std::bitset<sizeof(uint32_t) * CHAR_BIT> flags_;
};

} // namespace wut
