/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */
#include <wut/entity.h>

#include <algorithm>
#include <cassert>
#include <wut/component.h>

namespace wut {
auto Entity::createRoot() -> EntityPtr {
  auto e = std::make_shared<Entity>();
  e->root_ = e;
  return e;
}

void Entity::addComponent(ComponentPtr const& component) {
  assert(component != nullptr);
  assert(component->parent_ == nullptr && "A component can only be added to one entity.");

  components_.push_back(component);
  // Order components by their update order.
  std::sort(components_.begin(), components_.end(), [](auto const& lhs, auto const& rhs) {
    return lhs->updateOrder() < rhs->updateOrder();
  });

  [[maybe_unused]] auto [_, inserted] = typeToComponents_.emplace(component->componentType(), component);
  assert(inserted && "A component with that type already exists on this entity.");

  component->parent_ = this;

  component->onAttach();
  if (flags_.test(detail::FLAG_ENABLED_IN_TREE) && component->enabled()) {
    component->onEnabled();
  }
}

auto Entity::component(std::type_index type) const -> ComponentPtr {
  auto it = typeToComponents_.find(type);
  if (it != typeToComponents_.end()) {
    return it->second;
  }

  return nullptr;
}

void Entity::destroy() {
  // Mark this entity as destroyed
  flags_.set(detail::FLAG_DESTROY);

  // Disable us and any children, firing onDisabled events if we were enabled in the tree.
  setEnabled(false);
}

void Entity::setEnabled(bool enabled) {
  if (flags_.test(detail::FLAG_ENABLED) == enabled) {
    return;
  }

  flags_.set(detail::FLAG_ENABLED, enabled);

  updateEnabledRecurse(enabled);
}

void Entity::setParent(EntityPtr const& parent) {
  assert(parent_ == nullptr && "An entity can only ever have one parent.");
  assert(parent != nullptr && "Parent must not be null");

  parent_ = parent.get();
  root_ = parent->root_;
  parent_->children_.push_back(shared_from_this());

  // If our parent is active in the tree then we should also be enabled in the tree.
  if (enabledSelf() && parent_->enabledInTree()) {
    updateEnabledRecurse(true);
  }
}

void Entity::update() {
  // Iterate by index since components may be added while we are iterating, when a component is added update will be
  // fired on the the frame.
  for (size_t i = 0; i < componentCount_; ++i) {
    auto&& component = components_[i];

    if (component->enabled()) {
      component->update();
    }
  }

  // Iterate by index since children may be added while we are iterating. Added children will only be updated on the
  // next frame.
  for (size_t i = 0; i < childCount_; ++i) {
    auto&& child = children_[i];

    if (child->enabledSelf()) {
      child->update();
    }
  }
}

void Entity::postUpdate() {
  bool needsComponentRemoval{false};
  // Iterate by index since components may be added while we are iterating, when a component is added update will be
  // fired on the the frame.
  for (size_t i = 0; i < componentCount_; ++i) {
    auto&& component = components_[i];

    if (component->flags_.test(detail::FLAG_DESTROY)) {
      needsComponentRemoval = true;
      continue;
    }

    if (component->enabled()) {
      component->postUpdate();
    }
  }

  bool needsChildRemoval{false};
  // Iterate by index since children may be added while we are iterating. Added children will only be updated on the
  // next frame.
  for (size_t i = 0; i < childCount_; ++i) {
    auto&& child = children_[i];
    if (child->flags_.test(detail::FLAG_DESTROY)) {
      // found a dead child, so we need to run some cleanup.
      needsChildRemoval = true;
      continue;
    }

    if (child->enabledSelf()) {
      child->postUpdate();
    }
  }

  if (needsComponentRemoval) {
    std::erase_if(components_, [](auto const& c) { return c->flags_.test(detail::FLAG_DESTROY); });
  }

  if (needsChildRemoval) {
    std::erase_if(children_, [](auto const& c) { return c->flags_.test(detail::FLAG_DESTROY); });
  }

  // Update our counts to include any components/entities that were added or removed.
  componentCount_ = components_.size();
  childCount_ = children_.size();
}

void Entity::updateEnabledRecurse(bool newState) {
  // If we are enabled, have a parent, and the parent is enabled in the tree then we are now active in the tree and
  // onEnabled should be fired.
  if (newState && parent_ && parent_->flags_.test(detail::FLAG_ENABLED_IN_TREE)) {
    flags_.set(detail::FLAG_ENABLED_IN_TREE);
    for (auto&& comp : components_) {
      // Fire onEnabled for all enabled components.
      if (comp->enabled()) {
        comp->onEnabled();
      }
    }
  }

  // If we are disabled, but we were active in the tree then we are no longer enabled in the tree and onDisabled should
  // be fired.
  if (!newState && flags_.test(detail::FLAG_ENABLED_IN_TREE)) {
    flags_.set(detail::FLAG_ENABLED_IN_TREE, false);
    // Fire onDisabled for all enabled components.
    for (auto&& comp : components_) {
      if (comp->enabled()) {
        comp->onDisabled();
      }
    }
  }

  // Recursively update all enabled children.
  for (auto&& child : children_) {
    if (child->enabledSelf()) {
      child->updateEnabledRecurse(newState);
    }
  }
}

} // namespace wut
