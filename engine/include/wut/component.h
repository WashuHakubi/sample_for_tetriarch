/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <bitset>
#include <typeindex>
#include <wut/fwd.h>
#include <wut/serialization.h>

namespace wut {
class Component {
 public:
  Component();

 public:
  /**
   * Gets the final type of this component.
   */
  virtual auto componentType() const -> std::type_index = 0;

  /**
   * True if the component is enabled.
   */
  auto enabled() const { return flags_.test(detail::FLAG_ENABLED); }

  /**
   * Returns a strong reference to the parent of this component, or nullptr if the component does not have a parent.
   */
  auto parent() const -> EntityPtr;

  /**
   * Indicates the preferred update order between components on the same entity. Lower values will be updated before
   * higher values.
   */
  virtual auto updateOrder() const -> int { return 0; }

 public:
  /**
   * Schedules this component to be destroyed by the end of the next update.
   */
  void destroy();

  /**
   * Changes the enabled state. If the parent of this component is enabled in the tree then onEnabled or onDisabled will
   * be fired.
   */
  void setEnabled(bool enabled);

 public:
  /**
   * Always invoked when a component is added to an entity.
   */
  virtual void onAttach() {}

  /**
   * Invoked when the entity is enabled in the tree and this component is enabled.
   */
  virtual void onEnabled() {}

  /**
   * Invoked exactly once, before the first update.
   */
  virtual void onStart() {}

  /**
   * Invoked when the entity is diabled in the tree or this component is disabled.
   */
  virtual void onDisabled() {}

  /**
   * Invoked once per frame. If the component is disabled or the entity the component is on is not enabled in the tree
   * then update will not be called.
   */
  virtual void update() {}

  /**
   * Invoked once per frame. If the component is disabled or the entity the component is on is not enabled in the tree
   * then postUpdate will not be called.
   */
  virtual void postUpdate() {}

  virtual void serialize(IWriter& writer, std::string_view name) const = 0;

  virtual void deserialize(IReader& reader, std::string_view name) = 0;

 private:
  friend class ComponentIterator;
  friend class Entity;

  Entity* parent_{nullptr};
  std::bitset<sizeof(uint32_t) * CHAR_BIT> flags_;
};

template <>
inline void writeSpecialized(IWriter& writer, std::string_view name, const Component& obj) {
  obj.serialize(writer, name);
}

template <>
inline void readSpecialized(IReader& reader, std::string_view name, Component& obj) {
  obj.deserialize(reader, name);
}

/**
 * Utility base class for components, allows specifying the component type and update order as template parameters.
 */
template <class T, size_t UpdateOrder = 0>
class ComponentT : public Component, public std::enable_shared_from_this<T> {
 public:
  auto componentType() const -> std::type_index final {
    static_assert(
        std::is_final_v<T> && std::is_base_of_v<ComponentT<T, UpdateOrder>, T> && IsSerializableClass<T>,
        "T must derive from ComponentT<T>, must implement IsSerializableCLass, and must be final.");
    return typeid(T);
  }

  auto updateOrder() const -> int final { return UpdateOrder; }

 protected:
  void serialize(IWriter& writer, std::string_view name) const override {
    writeObject(writer, name, *static_cast<T const*>(this));
  }

  void deserialize(IReader& reader, std::string_view name) override {
    readObject(reader, name, *static_cast<T*>(this));
  }
};

} // namespace wut
