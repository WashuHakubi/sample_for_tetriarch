/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */
#include <wut/entity.h>

#include <algorithm>
#include <cassert>
#include <wut/component.h>
#include <wut/scoped.h>

namespace wut {

auto Entity::createRoot() -> EntityPtr {
  auto e = std::make_shared<Entity>(InternalOnly{});
  e->flags_ |= EntityFlags{Flags::IsRoot} | Flags::Enabled | Flags::EnabledInTree;
  e->root_ = e;
  return e;
}

auto Entity::create(std::string name, std::shared_ptr<Entity> const& parent) -> EntityPtr {
  auto e = std::make_shared<Entity>(InternalOnly{});
  e->name_ = std::move(name);
  // All entities are enabled by default.
  e->flags_.set(Flags::Enabled);

  if (parent) {
    e->setParent(parent);
  }

  return e;
}

auto Entity::createEmpty() -> EntityPtr {
  return create("");
}

Entity::Entity(InternalOnly const&) {}

auto Entity::addComponent(ComponentPtr const& component) -> ComponentPtr {
  assert(component != nullptr);
  assert(component->parent_ == nullptr && "A component can only be added to one entity.");

  components_.push_back(component);
  if (!flags_.test(Flags::IsUpdating)) {
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
  if (flags_.test(Flags::EnabledInTree) && component->enabled()) {
    component->onEnabled();
  }

  return component;
}

auto Entity::component(std::type_index type) const -> ComponentPtr {
  auto it = typeToComponents_.find(type);
  if (it != typeToComponents_.end()) {
    return it->second;
  }

  return nullptr;
}

void Entity::destroy() {
  assert(!flags_.test(Flags::IsRoot) && "Cannot destroy root object");
  // Mark this entity as destroyed
  flags_.set(Flags::Destroy);

  // Disable us and any children, firing onDisabled events if we were enabled in the tree.
  setEnabled(false);
}

void Entity::setEnabled(bool enabled) {
  // If enabled is the same or we're the root object (in which case we cannot change the enabled state), just return.
  if (flags_.test(Flags::IsRoot) || flags_.test(Flags::Enabled) == enabled) {
    return;
  }

  flags_.set(Flags::Enabled, enabled);

  updateEnabledRecurse(enabled);
}

void Entity::setParent(EntityPtr const& parent) {
  assert(parent_ == nullptr && "An entity can only ever have one parent.");
  assert(parent != nullptr && "Parent must not be null");
  assert(!flags_.test(Flags::IsRoot) && "Root object cannot have a parent.");

  parent_ = parent.get();
  root_ = parent->root_;
  parent_->children_.push_back(shared_from_this());

  // If our parent is active in the tree then we should also be enabled in the tree.
  if (enabledSelf() && parent_->enabledInTree()) {
    updateEnabledRecurse(true);
  }
}

void Entity::update() {
  // Mark us as updating. This prevents certain changes, such as component count updating
  flags_.set(Flags::IsUpdating);
  Scoped s{[this]() { flags_.clear(Flags::IsUpdating); }};

  // Iterate by index since components may be added while we are iterating, when a component is added update will be
  // fired on the the frame.
  for (size_t i = 0; i < componentCount_; ++i) {
    auto&& component = components_[i];

    if (component->enabled()) {
      // onStart should be called exactly once during the lifetime of the component.
      if (!component->flags_.test(Component::Flags::Started)) {
        component->flags_.set(Component::Flags::Started);
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
}

void Entity::postUpdate() {
  // Mark us as updating. This prevents certain changes, such as component count updating
  flags_.set(Flags::IsUpdating);
  Scoped s{[this]() { flags_.clear(Flags::IsUpdating); }};

  bool needsComponentRemoval{false};
  // Iterate by index since components may be added while we are iterating, when a component is added update will be
  // fired on the the frame.
  for (size_t i = 0; i < components_.size(); ++i) {
    auto&& component = components_[i];

    if (component->flags_.test(Component::Flags::Destroy)) {
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
    if (child->flags_.test(Flags::Destroy)) {
      // found a dead child, so we need to run some cleanup.
      needsChildRemoval = true;
      continue;
    }

    if (child->enabledSelf()) {
      child->postUpdate();
    }
  }

  // If components were added during the update then we need to reorder them.
  if (componentCount_ != components_.size()) {
    std::sort(components_.begin(), components_.end(), [](auto const& lhs, auto const& rhs) {
      return lhs->updateOrder() < rhs->updateOrder();
    });
  }

  if (needsComponentRemoval) {
    std::erase_if(components_, [](auto const& c) { return c->flags_.test(Component::Flags::Destroy); });
  }

  if (needsChildRemoval) {
    std::erase_if(children_, [](auto const& c) { return c->flags_.test(Flags::Destroy); });
  }

  componentCount_ = components_.size();
}

void Entity::updateEnabledRecurse(bool newState) {
  // If we are enabled, have a parent, and the parent is enabled in the tree then we are now active in the tree and
  // onEnabled should be fired.
  if (newState && parent_ && parent_->enabledInTree()) {
    flags_.set(Flags::EnabledInTree);
    for (auto&& comp : components_) {
      // Fire onEnabled for all enabled components.
      if (comp->enabled()) {
        comp->onEnabled();
      }
    }
  }

  // If we are disabled, but we were active in the tree then we are no longer enabled in the tree and onDisabled should
  // be fired.
  if (!newState && flags_.test(Flags::EnabledInTree)) {
    flags_.set(Flags::EnabledInTree, false);
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
ComponentIterator::ComponentIterator(std::shared_ptr<Entity const> entity, size_t index)
    : entity_(std::move(entity))
    , index_(index) {
  // Find the next non-destroyed component if we're not already at the end of the set of components.
  auto const& components = entity_->components_;
  while (index_ < components.size() && components[index_]->flags_.test(Component::Flags::Destroy)) {
    ++index_;
  }
}

ComponentPtr const& ComponentIterator::operator*() const {
  auto const& components = entity_->components_;
  assert(index_ < components.size());

  return components[index_];
}

ComponentIterator& ComponentIterator::operator++() {
  auto const& components = entity_->components_;
  ++index_;

  // Skip over any components that are dead.
  while (index_ < components.size() && components[index_]->flags_.test(Component::Flags::Destroy)) {
    ++index_;
  }

  return *this;
}

EntityIterator::EntityIterator(std::shared_ptr<Entity const> entity, size_t index)
    : entity_(std::move(entity))
    , index_(index) {
  auto const& entities = entity_->children_;
  // Skip over any entities that are dead.
  while (index_ < entities.size() && entities[index_]->flags_.test(Entity::Flags::Destroy)) {
    ++index_;
  }
}

EntityPtr const& EntityIterator::operator*() const {
  auto const& entities = entity_->children_;
  assert(index_ < entities.size());

  return entities[index_];
}

EntityIterator& EntityIterator::operator++() {
  auto const& entities = entity_->children_;
  ++index_;

  // Skip over any entities that are dead.
  while (index_ < entities.size() && entities[index_]->flags_.test(Entity::Flags::Destroy)) {
    ++index_;
  }

  return *this;
}
void DeserializeObserver<Entity>::onDeserialized(Entity& obj) {
  obj.componentCount_ = obj.components_.size();
  for (auto&& comp : obj.components_) {
    assert(comp);
    obj.typeToComponents_.emplace(comp->componentType(), comp);
  }
}

void writeObject(IWriter& writer, std::string_view name, const Entity::EntityFlags& obj, WriteTags& tags) {
  writer.beginObject(name);
  writer.write("enabled", obj.test(Entity::Flags::Enabled));
  writer.endObject();
}

void readObject(IReader& reader, std::string_view name, Entity::EntityFlags& obj, ReadTags& tags) {
  reader.beginObject(name);
  bool v;
  reader.read("enabled", v);
  reader.endObject();

  obj.set(Entity::Flags::Enabled, v);
}

} // namespace wut
