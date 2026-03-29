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
} // namespace wut
