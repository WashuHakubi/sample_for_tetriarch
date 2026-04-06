/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <span>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <wut/flags.h>
#include <wut/fwd.h>
#include <wut/serialization.h>
#include <wut/transform.h>

namespace wut {
class ComponentIterator {
  ComponentIterator(std::shared_ptr<Entity const> entity, size_t index);

 public:
  using difference_type = std::ptrdiff_t;
  using value_type = ComponentPtr;

  ComponentIterator(ComponentIterator const&) = default;

  ComponentIterator& operator=(ComponentIterator const&) = default;

  ComponentPtr const& operator*() const;

  ComponentIterator& operator++();

  ComponentIterator operator++(int) {
    auto result = *this;
    ++*this;
    return result;
  }

  bool operator==(ComponentIterator const& other) const { return entity_ == other.entity_ && index_ == other.index_; }

  bool operator!=(ComponentIterator const& other) const { return !(*this == other); }

 private:
  friend class Entity;
  std::shared_ptr<Entity const> entity_;
  size_t index_;
};

class ComponentIterable {
 public:
  ComponentIterable(ComponentIterator begin, ComponentIterator end) : begin_(std::move(begin)), end_(std::move(end)) {}

  auto begin() const { return begin_; }
  auto end() const { return end_; }

 private:
  friend class Entity;

  ComponentIterator begin_;
  ComponentIterator end_;
};

class EntityIterator {
  EntityIterator(std::shared_ptr<Entity const> entity, size_t index);

 public:
  using difference_type = std::ptrdiff_t;
  using value_type = EntityPtr;

  EntityIterator(EntityIterator const&) = default;

  EntityIterator& operator=(EntityIterator const&) = default;

  EntityPtr const& operator*() const;

  EntityIterator& operator++();

  EntityIterator operator++(int) {
    auto result = *this;
    ++*this;
    return result;
  }

  bool operator==(EntityIterator const& other) const { return entity_ == other.entity_ && index_ == other.index_; }

  bool operator!=(EntityIterator const& other) const { return !(*this == other); }

 private:
  friend class Entity;
  std::shared_ptr<Entity const> entity_;
  size_t index_;
};

class Entity : public std::enable_shared_from_this<Entity> {
  struct InternalOnly {};

 public:
  enum class Flags {
    Enabled = 0,
    Destroy = 1,
    EnabledInTree = 2,
    IsRoot = 3,
    IsUpdating = 4,
  };
  using EntityFlags = wut::Flags<Flags>;

  static auto createRoot() -> EntityPtr;

  static auto create(std::string name, std::shared_ptr<Entity> const& parent = nullptr) -> EntityPtr;

  static auto createEmpty() -> EntityPtr;

  static auto serializeMembers() {
    return std::make_tuple(
        std::make_tuple("name", &Entity::name_),
        std::make_tuple("transform", &Entity::transform_),
        std::make_tuple("children", &Entity::children_),
        std::make_tuple("components", &Entity::components_),
        std::make_tuple("flags", &Entity::flags_));
  }
  Entity(InternalOnly const&);

 public:
  auto children() const -> std::span<EntityPtr const> { return children_; }

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
   * Gets a pair of iterators that can be used to iterate over all the components on this entity.
   * If called outside of update, any removal of components on this entity will result in undefined behaviour, and
   * component ordering may change unpredictably if components are added during traversal.
   */
  auto components() const -> ComponentIterable {
    return ComponentIterable{
        ComponentIterator(shared_from_this(), 0),
        ComponentIterator(shared_from_this(), components_.size())};
  }

  /**
   * Gets an iterator to the first valid child object.
   *
   * If called outside of update, any removal of a child on this entity will result in undefined behaviour.
   */
  auto begin() const { return EntityIterator{shared_from_this(), 0}; }

  /**
   * Gets an iterator to the end of the child objects collection.
   *
   * If called outside of update, any removal of a child on this entity will result in undefined behaviour.
   */
  auto end() const { return EntityIterator{shared_from_this(), children_.size()}; }

  /**
   * True if the entity is in the scene, the entity is enabled, and all ancestors are also enabled.
   */
  auto enabledInTree() const { return flags_.test(Flags::EnabledInTree); }

  /**
   * True if the entity is enabled locally.
   */
  auto enabledSelf() const { return flags_.test(Flags::Enabled); }

  /**
   * Name of this object
   */
  auto name() const -> std::string_view { return name_; }

  /**
   * Returns a strong reference to the parent of this entity, or nullptr if the entity does not have a parent.
   */
  auto parent() const { return parent_ ? parent_->shared_from_this() : nullptr; }

  /**
   * Returns a strong reference to the root of the tree, or nullptr if this is not in the tree.
   */
  auto root() const { return root_.lock(); }

  /**
   * Gets the transform for this entity.
   */
  auto transform() const -> Transform const& { return transform_; }

  /**
   * Gets the transform for this entity.
   */
  auto transform() -> Transform& { return transform_; }

 public:
  template <class T>
  auto addComponent(std::shared_ptr<T> const& component) -> std::shared_ptr<T> {
    addComponent(std::static_pointer_cast<Component>(component));
    return component;
  }

  /**
   * Adds a component to this entity. If this entity is enabled in the tree and the component is enabled then
   * onEnabled() will be fired.
   */
  auto addComponent(ComponentPtr const& component) -> ComponentPtr;

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
  friend class ComponentIterator;
  friend class EntityIterator;
  friend struct DeserializeObserver<Entity>;

  Entity* parent_{nullptr};
  EntityHandle root_{};
  Transform transform_;
  EntityFlags flags_;
  // std::bitset<sizeof(uint32_t) * CHAR_BIT> flags_;

  std::vector<EntityPtr> children_;

  std::vector<ComponentPtr> components_;
  size_t componentCount_{0};

  std::unordered_map<std::type_index, ComponentPtr> typeToComponents_;
  std::string name_;
};

void writeObject(IWriter& writer, std::string_view name, const Entity::EntityFlags& obj, WriteTags& tags);

void readObject(IReader& reader, std::string_view name, Entity::EntityFlags& obj, ReadTags& tags);

template <>
struct SerializeFactory<Entity> {
  static auto create() { return Entity::createEmpty(); }
};

template <>
struct DeserializeObserver<Entity> {
  static void onDeserialized(Entity& obj);
};
} // namespace wut
