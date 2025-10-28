/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once
#include <memory>
#include <vector>

#include "entity.h"

namespace ew {
struct IArchetypeStorage;
using ArchetypeStoragePtr = std::unique_ptr<IArchetypeStorage>;

struct IArchetypeStorage {
  virtual ~IArchetypeStorage() = default;

  // Gets the component id for the component this storage contains.
  [[nodiscard]] virtual ComponentId getId() const = 0;

  // Returns a pointer to the start of the component storage.
  [[nodiscard]] virtual void* data() = 0;

  // Allocates a component and returns the index.
  [[nodiscard]] virtual size_t alloc() = 0;

  // Copies the component from `source` at `srcIndex` into this storage at `dstIndex`
  virtual void setFrom(ArchetypeStoragePtr const& source, size_t srcIndex, size_t dstIndex) = 0;
};

template <class T>
struct ArchetypeStorage final : IArchetypeStorage {
  [[nodiscard]] ComponentId getId() const override { return getComponentId<T>(); }

  [[nodiscard]] void* data() override { return data_.data(); }

  [[nodiscard]] size_t alloc() override {
    data_.emplace_back();
    return data_.size() - 1;
  }

  void setFrom(ArchetypeStoragePtr const& source, size_t srcIndex, size_t dstIndex) override {
    assert(dstIndex < data_.size());
    assert(source->getId() == getId());
    auto srcPtr = static_cast<ArchetypeStorage<T> const*>(source.get());
    assert(srcIndex < srcPtr->data_.size());

    data_[dstIndex] = srcPtr->data_[srcIndex];
  }

  std::vector<T> data_;
};
} // namespace ew