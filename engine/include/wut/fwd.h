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
enum EntityFlags {
  FLAG_ENABLED = 0x01,
  FLAG_ENABLED_IN_TREE = 0x02,
  FLAG_DESTROY = 0x04,
};
}
} // namespace wut
