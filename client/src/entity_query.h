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
    static void build(
        std::unordered_set<ComponentId>& requiredComponents,
        std::unordered_set<ComponentId>& writeComponents,
        std::unordered_set<ComponentId>& allComponents);
  };

  template <class ArgTuple>
  struct ComponentFnDetails<ArgTuple, 0> {
    static void build(
        std::unordered_set<ComponentId>& requiredComponents,
        std::unordered_set<ComponentId>& writeComponents,
        std::unordered_set<ComponentId>& allComponents);
  };

 public:
  explicit EntityQuery(std::vector<ArchetypePtr> const* archetypes) : archetypes_(archetypes) {}

  EntityQuery& with(std::initializer_list<ComponentId> ids);

  template <class T>
  EntityQuery& with() {
    return with({getComponentId<T>()});
  }

  template <class T, class T2, class... Ts>
  EntityQuery& with();

  EntityQuery& without(std::initializer_list<ComponentId> ids);

  template <class T>
  EntityQuery& without();

  template <class T, class T2, class... Ts>
  EntityQuery& without();

  EntityQuery& atLeastOneOf(std::initializer_list<ComponentId> ids);

  template <class T>
  EntityQuery& atLeastOneOf();

  template <class T, class T2, class... Ts>
  EntityQuery& atLeastOneOf();

  template <class Fn>
  void forEach(Fn const& fn);

 private:
  EntityQuery& addComponents(ComponentSet& set, std::initializer_list<ComponentId> ids);

  std::vector<ArchetypePtr> const* archetypes_;

  std::unordered_set<ComponentId> allComponents_;
  std::unordered_set<ComponentId> writeComponents_;

  std::unordered_set<ComponentId> requiredComponents_;
  std::unordered_set<ComponentId> withoutComponents_;
  std::unordered_set<ComponentId> atLeastOneComponents_;
};

template <class ArgTuple, int I>
void EntityQuery::ComponentFnDetails<ArgTuple, I>::build(
    std::unordered_set<ComponentId>& requiredComponents,
    std::unordered_set<ComponentId>& writeComponents,
    std::unordered_set<ComponentId>& allComponents) {
  static_assert(I > 0, "Function must take at least one component.");

  using SigTraits = detail::FnArgTraits<ArgTuple, I>;
  using ElementType = SigTraits::ElementType;

  allComponents.emplace(getComponentId<ElementType>());
  if constexpr (!SigTraits::isOptional) {
    requiredComponents.emplace(getComponentId<ElementType>());
  }

  if constexpr (!SigTraits::isReadOnly) {
    writeComponents.emplace(getComponentId<ElementType>());
  }

  ComponentFnDetails<ArgTuple, I - 1>::build(requiredComponents, writeComponents, allComponents);
}

template <class ArgTuple>
void EntityQuery::ComponentFnDetails<ArgTuple, 0>::build(
    std::unordered_set<ComponentId>& requiredComponents,
    std::unordered_set<ComponentId>& writeComponents,
    std::unordered_set<ComponentId>& allComponents) {
  using SigTraits = detail::FnArgTraits<ArgTuple, 0>;
  using ElementType = SigTraits::ElementType;

  allComponents.emplace(getComponentId<ElementType>());
  if constexpr (!SigTraits::isOptional) {
    requiredComponents.emplace(getComponentId<ElementType>());
  }

  if constexpr (!SigTraits::isReadOnly) {
    writeComponents.emplace(getComponentId<ElementType>());
  }
}

template <class T, class T2, class... Ts>
EntityQuery& EntityQuery::with() {
  return with({getComponentId<T>(), getComponentId<T2>(), getComponentId<Ts>()...});
}

template <class T>
EntityQuery& EntityQuery::without() {
  return without({getComponentId<T>()});
}

template <class T, class T2, class... Ts>
EntityQuery& EntityQuery::without() {
  return without({getComponentId<T>(), getComponentId<T2>(), getComponentId<Ts>()...});
}

template <class T>
EntityQuery& EntityQuery::atLeastOneOf() {
  return atLeastOneOf({getComponentId<T>()});
}

template <class T, class T2, class... Ts>
EntityQuery& EntityQuery::atLeastOneOf() {
  return atLeastOneOf({getComponentId<T>(), getComponentId<T2>(), getComponentId<Ts>()...});
}

template <class Fn>
void EntityQuery::forEach(Fn const& fn) {
  using Sig = detail::FnSig<Fn>;

  ComponentFnDetails<typename Sig::ArgTuple>::build(requiredComponents_, writeComponents_, allComponents_);

  for (auto&& arch : *archetypes_) {
    if (arch->match(requiredComponents_, withoutComponents_, atLeastOneComponents_)) {
      arch->forEach(fn);
    }
  }
}
} // namespace ew
