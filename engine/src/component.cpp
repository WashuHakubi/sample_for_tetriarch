/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/component.h>

#include <wut/entity.h>

namespace wut {
Component::Component() {
  flags_.set(detail::FLAG_ENABLED);
}

void Component::destroy() {
  // Mark this component for destruction
  flags_.set(detail::FLAG_DESTROY);

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
  if (flags_.test(detail::FLAG_ENABLED) == enabled) {
    return;
  }

  flags_.set(detail::FLAG_ENABLED, enabled);

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

} // namespace wut
