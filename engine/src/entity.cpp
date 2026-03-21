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
  auto e = std::make_shared<Entity>(InternalOnly{});
  e->flags_.set(detail::FLAG_ENTITY_IS_ROOT);
  e->flags_.set(detail::FLAG_ENTITY_ENABLED_IN_TREE);
  e->flags_.set(detail::FLAG_ENABLED);
  e->root_ = e;
  return e;
}

auto Entity::create(std::shared_ptr<Entity> const& parent) -> EntityPtr {
  auto e = std::make_shared<Entity>(InternalOnly{});
  // All entities are enabled by default.
  e->flags_.set(detail::FLAG_ENABLED);

  if (parent) {
    e->setParent(parent);
  }

  return e;
}

Entity::Entity(InternalOnly const&) {}

void Entity::addComponent(ComponentPtr const& component) {
  assert(component != nullptr);
  assert(component->parent_ == nullptr && "A component can only be added to one entity.");

  components_.push_back(component);
  if (!flags_.test(detail::FLAG_ENTITY_IS_UPDATING)) {
    // If we're not updating this object then we can simply update the component order and count
    std::sort(components_.begin(), components_.end(), [](auto const& lhs, auto const& rhs) {
      return lhs->updateOrder() < rhs->updateOrder();
    });
    componentCount_ = components_.size();
  }

  [[maybe_unused]] auto [_, inserted] = typeToComponents_.emplace(component->componentType(), component);
  assert(inserted && "A component with that type already exists on this entity.");

  component->parent_ = this;

  component->onAttach();
  if (flags_.test(detail::FLAG_ENTITY_ENABLED_IN_TREE) && component->enabled()) {
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
  // If enabled is the same or we're the root object (in which case we cannot change the enabled state), just return.
  if (flags_.test(detail::FLAG_ENTITY_IS_ROOT) || flags_.test(detail::FLAG_ENABLED) == enabled) {
    return;
  }

  flags_.set(detail::FLAG_ENABLED, enabled);

  updateEnabledRecurse(enabled);
}

void Entity::setParent(EntityPtr const& parent) {
  assert(parent_ == nullptr && "An entity can only ever have one parent.");
  assert(parent != nullptr && "Parent must not be null");
  assert(!flags_.test(detail::FLAG_ENTITY_IS_ROOT) && "Root object cannot have a parent.");

  parent_ = parent.get();
  root_ = parent->root_;
  parent_->children_.push_back(shared_from_this());

  // If our parent is active in the tree then we should also be enabled in the tree.
  if (enabledSelf() && parent_->enabledInTree()) {
    updateEnabledRecurse(true);
  }
}

void Entity::update() {
  flags_.set(detail::FLAG_ENTITY_IS_UPDATING);

  // Iterate by index since components may be added while we are iterating, when a component is added update will be
  // fired on the the frame.
  for (size_t i = 0; i < componentCount_; ++i) {
    auto&& component = components_[i];

    if (component->enabled()) {
      // onStart should be called exactly once during the lifetime of the component.
      if (!component->flags_.test(detail::FLAG_COMP_STARTED)) {
        component->flags_.set(detail::FLAG_COMP_STARTED);
        component->onStart();
      }

      component->update();
    }
  }

  // Iterate by index since children may be added while we are iterating.
  for (size_t i = 0; i < children_.size(); ++i) {
    auto&& child = children_[i];

    if (child->enabledSelf()) {
      child->update();
    }
  }

  flags_.set(detail::FLAG_ENTITY_IS_UPDATING, false);
}

void Entity::postUpdate() {
  flags_.set(detail::FLAG_ENTITY_IS_UPDATING);

  bool needsComponentRemoval{false};
  // Iterate by index since components may be added while we are iterating, when a component is added update will be
  // fired on the the frame.
  for (size_t i = 0; i < components_.size(); ++i) {
    auto&& component = components_[i];

    if (component->flags_.test(detail::FLAG_DESTROY)) {
      needsComponentRemoval = true;
      continue;
    }

    // Skip over components added after the update started. We still want to check if they need to be destroyed.
    if (i >= componentCount_) {
      continue;
    }

    if (component->enabled()) {
      component->postUpdate();
    }
  }

  bool needsChildRemoval{false};
  // Iterate by index since children may be added while we are iterating.
  for (size_t i = 0; i < children_.size(); ++i) {
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

  if (componentCount_ != components_.size()) {
    // Reorder the components by their update order and then set the count to the number of components.
    std::sort(components_.begin(), components_.end(), [](auto const& lhs, auto const& rhs) {
      return lhs->updateOrder() < rhs->updateOrder();
    });
    componentCount_ = components_.size();
  }

  flags_.set(detail::FLAG_ENTITY_IS_UPDATING, false);
}

void Entity::updateEnabledRecurse(bool newState) {
  // If we are enabled, have a parent, and the parent is enabled in the tree then we are now active in the tree and
  // onEnabled should be fired.
  if (newState && parent_ && parent_->flags_.test(detail::FLAG_ENTITY_ENABLED_IN_TREE)) {
    flags_.set(detail::FLAG_ENTITY_ENABLED_IN_TREE);
    for (auto&& comp : components_) {
      // Fire onEnabled for all enabled components.
      if (comp->enabled()) {
        comp->onEnabled();
      }
    }
  }

  // If we are disabled, but we were active in the tree then we are no longer enabled in the tree and onDisabled should
  // be fired.
  if (!newState && flags_.test(detail::FLAG_ENTITY_ENABLED_IN_TREE)) {
    flags_.set(detail::FLAG_ENTITY_ENABLED_IN_TREE, false);
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
