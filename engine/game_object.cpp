/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "engine/game_object.h"
#include "engine/component.h"
#include "engine/scoped.h"

#include <stack>

namespace ewok {
void GameObject::addChild(GameObjectPtr child) {
  if (updateState_ == UpdateState::Update) {
    postUpdateActions_.push_back([child = std::move(child)](GameObject* self) {
      self->addChild(std::move(child));
    });
    return;
  }

  assert(child->parent_ == nullptr);
  child->parent_ = this;

  // We're about to move the child ptr, so keep a copy around that's not moved.
  auto childPtr = child.get();
  children_.push_back(std::move(child));

  // If we're not lazy and the child is lazy then fire the attachment messages.
  if (!lazyAttach_ && childPtr->lazyAttach_) {
    childPtr->fireAttached();
  }
}

void GameObject::addComponent(ComponentPtr component) {
  if (updateState_ == UpdateState::Update) {
    postUpdateActions_.push_back(
        [component = std::move(component)](GameObject* self) {
          self->addComponent(std::move(component));
        });
    return;
  }

  assert(component->parent_ == nullptr);
  component->parent_ = this;

  if (component->hasUpdate()) {
    updateComponents_.insert(component.get());
  }

  if (component->hasPostUpdate()) {
    postUpdateComponents_.insert(component.get());
  }

  if (component->hasRender()) {
    renderables_.insert(component.get());
  }

  components_.push_back(component);

  if (!lazyAttach_) {
    // Notify the component that it has a new parent.
    component->attach();
  }
}

void GameObject::applyPostUpdateActions() {
  assert(updateState_ == UpdateState::Idle);
  for (auto&& action : postUpdateActions_) {
    action(this);
  }
  postUpdateActions_ = {};
}

void GameObject::fireAttached() {
  lazyAttach_ = false;

  for (auto&& component : components_) {
    component->attach();
  }

  // Run for all the child objects as well
  for (auto&& child : children_) {
    child->fireAttached();
  }
}

auto GameObject::path() const noexcept -> std::string {
  std::stack<GameObject const*> parts;

  GameObject const* cur = this;
  size_t needed = 0;

  // Walk up the tree of GOs till we get to the root.
  while (cur) {
    // Get how much space we need to avoid reallocating.
    needed += cur->name().size() + 1;

    // And push this object onto the stack.
    parts.push(cur);
    cur = cur->parent();
  }
  // We counted an extra / at the end, so remove it.
  --needed;

  std::string path;
  path.reserve(needed);

  while (!parts.empty()) {
    path.append(parts.top()->name());
    parts.pop();
    if (!parts.empty()) {
      path.append("/");
    }
  }

  // assert(path.size() == needed);

  return path;
}

void GameObject::postUpdate() {
  if (!active_) {
    return;
  }

  {
    updateState_ = UpdateState::Update;
    SCOPED([this]() { updateState_ = UpdateState::Idle; });

    for (auto&& component : postUpdateComponents_) {
      component->postUpdate();
    }

    for (auto&& child : children_) {
      child->postUpdate();
    }
  }

  applyPostUpdateActions();

  // If we have updated our active state during the update or post update status
  // then apply the new state at the end of postUpdate
  if (pendingActive_ != active_) {
    setActive(pendingActive_);
  }
}

void GameObject::queueRemoveChild(GameObjectPtr const& child) {
  postUpdateActions_.push_back([child = std::move(child)](GameObject* self) {
    self->removeChild(std::move(child));
  });
}

void GameObject::queueRemoveComponent(ComponentPtr const& component) {
  postUpdateActions_.push_back(
      [component = std::move(component)](GameObject* self) {
        self->removeComponent(std::move(component));
      });
}

void GameObject::registerForRender(ComponentPtr const& component) {
  if (updateState_ == UpdateState::Update) {
    postUpdateActions_.push_back([component = component](GameObject* self) {
      self->registerForRender(component);
    });
    return;
  }

  if (component->hasRender()) {
    renderables_.insert(component.get());
  }
}

void GameObject::registerForUpdate(ComponentPtr const& component) {
  if (updateState_ == UpdateState::Update) {
    postUpdateActions_.push_back([component = component](GameObject* self) {
      self->registerForUpdate(component);
    });
    return;
  }

  if (component->hasUpdate()) {
    updateComponents_.insert(component.get());
  }

  if (component->hasPostUpdate()) {
    postUpdateComponents_.insert(component.get());
  }
}

void GameObject::removeChild(GameObjectPtr const& child) {
  if (updateState_ == UpdateState::Update) {
    queueRemoveChild(child);
    return;
  }

  assert(child->parent_ == this);
  child->parent_ = nullptr;
  children_.erase(
      std::remove(children_.begin(), children_.end(), child), children_.end());
}

void GameObject::removeComponent(ComponentPtr const& component) {
  if (updateState_ == UpdateState::Update) {
    queueRemoveComponent(component);
    return;
  }

  assert(component->parent_ == this);
  component->parent_ = nullptr;
  components_.erase(
      std::remove(components_.begin(), components_.end(), component),
      components_.end());

  if (component->hasUpdate()) {
    updateComponents_.erase(component.get());
  }

  if (component->hasPostUpdate()) {
    postUpdateComponents_.erase(component.get());
  }

  // Notify the component that it no longer has a parent.
  component->detach();
}

void GameObject::render(Renderer& renderer) {
  if (!active_) {
    return;
  }

  {
    updateState_ = UpdateState::Update;
    SCOPED([this]() { updateState_ = UpdateState::Idle; });

    for (auto&& component : renderables_) {
      component->render(renderer);
    }

    for (auto&& child : children_) {
      child->render(renderer);
    }
  }

  applyPostUpdateActions();
}

void GameObject::setActive(bool active) {
  if (updateState_ == UpdateState::Idle) {
    // If we're idle then it is safe to immediately update our active state.
    active_ = pendingActive_ = active;
  } else {
    // Otherwise we're in update or postUpdate, so schedule it to happen at the
    // end of postUpdate.
    pendingActive_ = active;
  }
}

void GameObject::unregisterForRender(ComponentPtr const& component) {
  if (updateState_ == UpdateState::Update) {
    postUpdateActions_.push_back([component = component](GameObject* self) {
      self->unregisterForRender(component);
    });
    return;
  }

  renderables_.erase(component.get());
}

void GameObject::unregisterForUpdate(ComponentPtr const& component) {
  if (updateState_ == UpdateState::Update) {
    postUpdateActions_.push_back([component = component](GameObject* self) {
      self->unregisterForUpdate(component);
    });
    return;
  }

  updateComponents_.erase(component.get());
  postUpdateComponents_.erase(component.get());
}

void GameObject::update(float dt) {
  if (!active_) {
    return;
  }

  {
    updateState_ = UpdateState::Update;
    SCOPED([this]() { updateState_ = UpdateState::Idle; });

    for (auto&& component : updateComponents_) {
      component->update(dt);
    }

    for (auto&& child : children_) {
      child->update(dt);
    }
  }

  applyPostUpdateActions();
}

auto GameObject::findDescendant(std::span<std::string> const& pathParts)
    -> GameObjectPtr {
  // Searching for an empty path should result in the current object returning.
  if (pathParts.empty()) {
    return shared_from_this();
  }

  auto const& next = pathParts.front();
  for (auto&& child : children_) {
    if (child->name() == next) {
      return child->findDescendant(pathParts.subspan(1));
    }
  }

  // Failed to find the child matching the name.
  return nullptr;
}
} // namespace ewok
