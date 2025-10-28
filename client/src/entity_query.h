/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once
#include <unordered_set>
#include <vector>

#include "archetype.h"
#include "entity.h"

namespace ew {
class EntityQuery {
  template <class ArgTuple, int I = std::tuple_size_v<ArgTuple> - 1>
  struct ComponentFnDetails {
    static void build(ComponentSet& requiredComponents, ComponentSet& writeComponents, ComponentSet& allComponents);
  };

  template <class ArgTuple>
  struct ComponentFnDetails<ArgTuple, 0> {
    static void build(ComponentSet& requiredComponents, ComponentSet& writeComponents, ComponentSet& allComponents);
  };

 public:
  explicit EntityQuery(std::vector<ArchetypePtr> const* archetypes) : archetypes_(archetypes) {}

  EntityQuery& with(ComponentSet const& ids);

  template <class T, class... Ts>
  EntityQuery& with();

  EntityQuery& without(ComponentSet const& ids);

  template <class T, class... Ts>
  EntityQuery& without();

  template <class Fn>
  void forEach(Fn const& fn);

 private:
  std::vector<ArchetypePtr> const* archetypes_;

  ComponentSet allComponents_;
  ComponentSet writeComponents_;

  ComponentSet requiredComponents_;
  ComponentSet withoutComponents_;
};

template <class ArgTuple, int I>
void EntityQuery::ComponentFnDetails<ArgTuple, I>::build(
    ComponentSet& requiredComponents,
    ComponentSet& writeComponents,
    ComponentSet& allComponents) {
  static_assert(I > 0, "Function must take at least one component.");

  using SigTraits = detail::FnArgTraits<ArgTuple, I>;
  using ElementType = SigTraits::ElementType;

  set(allComponents, getComponentId<ElementType>());
  if constexpr (!SigTraits::isOptional) {
    set(requiredComponents, getComponentId<ElementType>());
  }

  if constexpr (!SigTraits::isReadOnly) {
    set(writeComponents, getComponentId<ElementType>());
  }

  ComponentFnDetails<ArgTuple, I - 1>::build(requiredComponents, writeComponents, allComponents);
}

template <class ArgTuple>
void EntityQuery::ComponentFnDetails<ArgTuple, 0>::build(
    ComponentSet& requiredComponents,
    ComponentSet& writeComponents,
    ComponentSet& allComponents) {
  using SigTraits = detail::FnArgTraits<ArgTuple, 0>;
  using ElementType = SigTraits::ElementType;

  set(allComponents, getComponentId<ElementType>());
  if constexpr (!SigTraits::isOptional) {
    set(requiredComponents, getComponentId<ElementType>());
  }

  if constexpr (!SigTraits::isReadOnly) {
    set(writeComponents, getComponentId<ElementType>());
  }
}

template <class T, class... Ts>
EntityQuery& EntityQuery::with() {
  set(requiredComponents_, getComponentId<T>());
  (set(requiredComponents_, getComponentId<Ts>()), ...);
  return *this;
}

template <class T, class... Ts>
EntityQuery& EntityQuery::without() {
  set(withoutComponents_, getComponentId<T>());
  (set(withoutComponents_, getComponentId<Ts>()), ...);
  return *this;
}

template <class Fn>
void EntityQuery::forEach(Fn const& fn) {
  using Sig = detail::FnSig<Fn>;

  // The lambda fn may impose additional requirements, these requirements are ephemeral to this function call
  auto requiredComponents = requiredComponents_;
  auto writeComponents = writeComponents_;
  auto allComponents = allComponents_;
  ComponentFnDetails<typename Sig::ArgTuple>::build(requiredComponents, writeComponents, allComponents);

  for (auto&& arch : *archetypes_) {
    if (arch->match(requiredComponents, withoutComponents_)) {
      arch->forEach(fn);
    }
  }
}
} // namespace ew
