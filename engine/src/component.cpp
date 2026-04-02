/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/component.h>

#include <ng-log/logging.h>
#include <wut/entity.h>

namespace wut {
Component::Component() {
  flags_.set(Flags::Enabled);
}

void Component::destroy() {
  // Mark this component for destruction
  flags_.set(Flags::Destroy);

  if (parent_) {
    // make sure we cannot be looked up by type in the parent.
    parent_->typeToComponents_.erase(componentType());
  }

  // Also disable the component to avoid updates getting fired. This may trigger onDisabled if the entity was enabled in
  // the tree.
  setEnabled(false);
}

auto Component::parent() const -> EntityPtr {
  return parent_ ? parent_->shared_from_this() : nullptr;
}

void Component::setEnabled(bool enabled) {
  if (flags_.test(Flags::Enabled) == enabled) {
    return;
  }

  flags_.set(Flags::Enabled, enabled);

  if (enabled && parent_->enabledInTree()) {
    onEnabled();
  }

  if (!enabled && parent_->enabledInTree()) {
    onDisabled();
  }
}

std::unordered_map<std::string_view, std::function<ComponentPtr()>> ComponentFactories::typeToFactory_;

void ComponentFactories::registerFactory(std::string_view name, std::function<ComponentPtr()> factory) {
  [[maybe_unused]] auto [_, inserted] = typeToFactory_.emplace(name, std::move(factory));
  assert(inserted);
}

ComponentPtr ComponentFactories::create(std::string_view name) {
  if (auto it = typeToFactory_.find(name); it != typeToFactory_.end()) {
    return it->second();
  }

  return nullptr;
}

void writeObject(IWriter& writer, std::string_view name, const ComponentPtr& obj, WriteTags& tags) {
  writer.beginObject(name);
  obj->serialize(writer, tags);
  writer.endObject();
}

void readObject(IReader& reader, std::string_view name, ComponentPtr& obj, ReadTags& tags) {
  reader.beginObject(name);
  std::string typeName;
  reader.read("$type", typeName);
  obj = ComponentFactories::create(typeName);
  if (!obj) {
    // Failed to find a type matching the typename.
    LOG(ERROR) << "Failed to create component with type name: " << typeName;
    reader.endObject();
    return;
  }

  obj->deserialize(reader, tags);
  reader.endObject();
}

void writeObject(IWriter& writer, std::string_view name, const Component::ComponentFlags& obj, WriteTags& tags) {
  writer.beginObject(name);
  writer.write("enabled", obj.test(Component::Flags::Enabled));
  writer.endObject();
}

void readObject(IReader& reader, std::string_view name, Component::ComponentFlags& obj, ReadTags& tags) {
  reader.beginObject(name);
  bool v;
  reader.read("enabled", v);
  reader.endObject();

  obj.set(Component::Flags::Enabled, v);
}

} // namespace wut
