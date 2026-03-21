/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <memory>

namespace wut {
class Component;
using ComponentPtr = std::shared_ptr<Component>;
using ComponentHandle = std::weak_ptr<Component>;

class Entity;
using EntityPtr = std::shared_ptr<Entity>;
using EntityHandle = std::weak_ptr<Entity>;

namespace detail {
// These represent indices into a bitset
enum EntityFlags {
  FLAG_ENABLED = 0x00,
  FLAG_DESTROY = 0x01,

  // Only valid on entities:
  FLAG_ENTITY_ENABLED_IN_TREE = 0x02,
  FLAG_ENTITY_IS_ROOT = 0x03,
  FLAG_ENTITY_IS_UPDATING = 0x04,

  // Only valid on components:
  FLAG_COMP_STARTED = 0x02,
};
} // namespace detail
} // namespace wut
