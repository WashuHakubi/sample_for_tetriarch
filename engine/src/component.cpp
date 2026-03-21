/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/component.h>

#include <wut/entity.h>

namespace wut {
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

auto Component::parent() const {
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
} // namespace wut
