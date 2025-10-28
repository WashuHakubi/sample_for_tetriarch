/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <memory>

#include "archetype_storage.h"
#include "entity.h"
#include "fn_traits.h"

namespace ew {
class Archetype;
using ArchetypePtr = std::shared_ptr<Archetype>;

class Archetype : public std::enable_shared_from_this<Archetype> {
  template <class ArgTuple, size_t N, size_t I = std::tuple_size_v<ArgTuple> - 1>
  struct BuildComponentPointers {
    static void build(Archetype& archetype, std::array<void*, N>& ptrs);
  };

  template <class ArgTuple, size_t N>
  struct BuildComponentPointers<ArgTuple, N, 0> {
    static void build(Archetype& archetype, std::array<void*, N>& ptrs);
  };

  template <class ArgTuple, size_t N>
  struct ConvertArg {
    using Traits = detail::FnArgTraits<ArgTuple, N>;

    static Traits::Type get(Archetype* self, std::array<void*, std::tuple_size_v<ArgTuple>> const& ptrs, size_t index);
  };

  template <class ArgTuple>
  struct Invoker {
    template <class Fn>
    static void invoke(
        Archetype* self,
        Fn const& fn,
        std::array<void*, std::tuple_size_v<ArgTuple>> const& ptrs,
        size_t index);

    template <class Fn, size_t... I>
    static void invokeInternal(
        Archetype* self,
        Fn const& fn,
        std::array<void*, std::tuple_size_v<ArgTuple>> const& ptrs,
        size_t index,
        std::integer_sequence<size_t, I...> seq);
  };

 public:
  explicit Archetype(std::unordered_map<ComponentId, ArchetypeStoragePtr> components, ComponentSet componentTypes);

  size_t allocate();

  void release(size_t id);

  template <class T>
  bool hasComponent() const {
    return hasComponent(getComponentId<T>());
  }

  bool hasComponent(const ComponentId id) const { return componentData_.contains(id); }

  bool match(ComponentSet const& with, ComponentSet const& without);

  template <class T>
  T* getComponents();

  template <class Fn>
  void forEach(Fn const& fn);

 private:
  friend class Archetypes;

  size_t nextId_{0};
  std::unordered_set<size_t> free_;
  std::unordered_map<ComponentId, ArchetypeStoragePtr> componentData_;
  ComponentSet components_;
};

template <class ArgTuple, size_t N, size_t I>
void Archetype::BuildComponentPointers<ArgTuple, N, I>::build(Archetype& archetype, std::array<void*, N>& ptrs) {
  using SigTraits = detail::FnArgTraits<ArgTuple, I>;
  using ElementType = SigTraits::ElementType;

  ptrs[I] = archetype.getComponents<ElementType>();

  assert(SigTraits::isOptional || ptrs[I] != nullptr);

  BuildComponentPointers<ArgTuple, N, I - 1>::build(archetype, ptrs);
}

template <class ArgTuple, size_t N>
void Archetype::BuildComponentPointers<ArgTuple, N, 0>::build(Archetype& archetype, std::array<void*, N>& ptrs) {
  using SigTraits = detail::FnArgTraits<ArgTuple, 0>;
  using ElementType = SigTraits::ElementType;

  ptrs[0] = archetype.getComponents<ElementType>();

  assert(SigTraits::isOptional || ptrs[0] != nullptr);
}

template <class ArgTuple, size_t N>
detail::FnArgTraits<ArgTuple, N>::Type Archetype::ConvertArg<ArgTuple, N>::get(
    Archetype* self,
    std::array<void*, std::tuple_size_v<ArgTuple>> const& ptrs,
    size_t index) {
  auto ptr = reinterpret_cast<Traits::ElementType*>(ptrs[N]);
  if constexpr (Traits::isOptional) {
    return ptr != nullptr ? &(ptr)[index] : nullptr;
  } else {
    assert(ptr != nullptr);
    return ptr[index];
  }
}

template <class ArgTuple>
template <class Fn>
void Archetype::Invoker<ArgTuple>::invoke(
    Archetype* self,
    Fn const& fn,
    std::array<void*, std::tuple_size_v<ArgTuple>> const& ptrs,
    size_t index) {
  auto seq = std::make_integer_sequence<size_t, std::tuple_size_v<ArgTuple>>{};

  invokeInternal(self, fn, ptrs, index, seq);
}

template <class ArgTuple>
template <class Fn, size_t... I>
void Archetype::Invoker<ArgTuple>::invokeInternal(
    Archetype* self,
    Fn const& fn,
    std::array<void*, std::tuple_size_v<ArgTuple>> const& ptrs,
    size_t index,
    std::integer_sequence<size_t, I...> seq) {
  std::invoke(fn, ConvertArg<ArgTuple, I>::get(self, ptrs, index)...);
}

template <class T>
T* Archetype::getComponents() {
  if (auto it = componentData_.find(getComponentId<T>()); it != componentData_.end()) {
    return static_cast<T*>(it->second->data());
  }

  return nullptr;
}

template <class Fn>
void Archetype::forEach(Fn const& fn) {
  if (nextId_ == 0)
    return;

  using Sig = detail::FnSig<Fn>;

  std::array<void*, Sig::argCount> ptrs{};
  BuildComponentPointers<typename Sig::ArgTuple, Sig::argCount>::build(*this, ptrs);

  for (size_t i = 0; i < nextId_; ++i) {
    if (free_.contains(i)) [[unlikely]]
      continue;

    Invoker<typename Sig::ArgTuple>::invoke(this, fn, ptrs, i);
  }
}
} // namespace ew
