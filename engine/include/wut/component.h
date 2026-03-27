/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <bitset>
#include <functional>
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

 private:
  template <class T, size_t>
  friend class ComponentT;
  friend class ComponentIterator;
  friend class Entity;

  friend void readObject(IReader& reader, std::string_view name, ComponentPtr& obj, ReadTags& tags);
  friend void writeObject(IWriter& writer, std::string_view name, const ComponentPtr& obj, WriteTags& tags);

  virtual void serialize(IWriter& writer, WriteTags& tags) const = 0;

  virtual void deserialize(IReader& reader, ReadTags& tags) = 0;

  Entity* parent_{nullptr};
  std::bitset<sizeof(uint32_t) * CHAR_BIT> flags_;
};

class ComponentFactories {
 public:
  static void registerFactory(std::string_view name, std::function<ComponentPtr()> factory);

  static ComponentPtr create(std::string_view name);

  static void reset() { typeToFactory_ = {}; }

 private:
  static std::unordered_map<std::string_view, std::function<ComponentPtr()>> typeToFactory_;
};

inline void writeObject(IWriter& writer, std::string_view name, const ComponentPtr& obj, WriteTags& tags) {
  writer.beginObject(name);
  obj->serialize(writer, tags);
  writer.endObject();
}

inline void readObject(IReader& reader, std::string_view name, ComponentPtr& obj, ReadTags& tags) {
  reader.beginObject(name);
  std::string typeName;
  reader.read("$type", typeName);
  obj = ComponentFactories::create(typeName);
  if (!obj) {
    // Failed to find a type matching the typename.
    // TODO: Add warning/error.
    reader.endObject();
    return;
  }

  obj->deserialize(reader, tags);
  reader.endObject();
}

template <class T>
concept IsComponent = requires {
  std::is_final_v<T> && std::is_base_of_v<Component, T> && detail::IsSerializableClass<T>;
  { T::typeName } -> std::same_as<std::string_view const&>;
};

/**
 * Utility base class for components, allows specifying the component type and update order as template parameters.
 */
template <class T, size_t UpdateOrder = 0>
class ComponentT : public Component, public std::enable_shared_from_this<T> {
 public:
  auto componentType() const -> std::type_index final {
    static_assert(
        IsComponent<T> && std::is_base_of_v<ComponentT<T, UpdateOrder>, T>,
        "T must derive from ComponentT<T>, and must implement IsComponent.");
    return typeid(T);
  }

  auto updateOrder() const -> int final { return UpdateOrder; }

 protected:
  void serialize(IWriter& writer, WriteTags& tags) const final {
    auto self = static_cast<T const*>(this);
    writer.write("$type", T::typeName);
    auto members = detail::getSerializeMembers<T>();
    detail::forEachTupleItem(members, [&writer, self, &tags](auto const& memberTuple) {
      auto& val = self->*std::get<1>(memberTuple);
      writeObject(writer, std::get<0>(memberTuple), val, tags);
    });
  }

  void deserialize(IReader& reader, ReadTags& tags) final {
    auto self = static_cast<T*>(this);
    auto members = detail::getSerializeMembers<T>();
    detail::forEachTupleItem(members, [&reader, self, &tags](auto const& memberTuple) {
      auto& val = self->*std::get<1>(memberTuple);
      readObject(reader, std::get<0>(memberTuple), val, tags);
    });
  }
};

} // namespace wut
