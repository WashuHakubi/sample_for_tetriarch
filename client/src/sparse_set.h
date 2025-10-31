/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <vector>

#include "entity_traits.h"

namespace ew {
template <class TEntity, class TEntityTraits = entity_traits<TEntity>>
class basic_sparse_set {
  using traits = TEntityTraits;
  using dense_container = std::vector<TEntity>;
  using sparse_container = std::vector<uint32_t*>;

  /**
   * Gets the index of the page and index in the page of an entity for the sparse array.
   */
  static auto entity_to_page_index(TEntity entity) {
    auto const v = traits::to_index(entity);
    return std::make_pair(v / traits::entities_per_page, v % traits::entities_per_page);
  }

  /**
   * Gets a pointer to the sparse entry for an entity or nullptr if the page does not exist.
   */
  auto* sparse_ptr(TEntity entity) const {
    auto [page, index] = entity_to_page_index(entity);

    return sparse_.size() > page && sparse_[page] ? sparse_[page] + index : nullptr;
  }

  /**
   * Gets a reference to a sparse entry for an entity. The sparse entry _must_ exist.
   */
  auto& sparse_ref(TEntity entity) const {
    auto ptr = sparse_ptr(entity);
    assert(ptr != nullptr);
    return *ptr;
  }

  /**
   * Ensures the sparse container has a page available for the entity and returns a reference to the entity's entry.
   */
  auto& grow_to_contain(TEntity entity) {
    auto [page, index] = entity_to_page_index(entity);

    if (sparse_.size() <= page) {
      // Ensure we have enough space in the sparse array to hold a pointer to the page
      sparse_.resize(page + 1);
    }

    if (sparse_[page] == nullptr) {
      // Allocate the page if needed and initialize it to the tombstone value
      sparse_[page] = new uint32_t[traits::entities_per_page];
      std::fill_n(sparse_[page], traits::entities_per_page, traits::tombstone);
    }

    return sparse_[page][index];
  }

  void release_all_sparse_pages() {
    for (auto&& page : sparse_) {
      delete[] page;
    }
    sparse_.clear();
  }

 public:
  using value_type = TEntity;
  using iterator = dense_container::const_iterator;

  basic_sparse_set() = default;

  basic_sparse_set(basic_sparse_set const&) = delete;
  basic_sparse_set(basic_sparse_set&&) noexcept = default;

  basic_sparse_set& operator=(basic_sparse_set const&) = delete;
  basic_sparse_set& operator=(basic_sparse_set&&) noexcept = default;

  iterator begin() const { return dense_.begin(); }

  void clear() {
    release_all_sparse_pages();
    dense_.clear();
  }

  bool contains(value_type entity) const {
    auto entity_index_ptr = sparse_ptr(entity);
    if (entity_index_ptr == nullptr || *entity_index_ptr == traits::tombstone) {
      return false;
    }
    return true;
  }

  iterator end() const { return dense_.end(); }

  void erase(value_type entity) {
    auto entity_index_ptr = sparse_ptr(entity);
    if (entity_index_ptr == nullptr || *entity_index_ptr == traits::tombstone) {
      return;
    }

    // A reference to the index of the last entry in the dense container. This could be the entity we're erasing.
    auto& last_entity_index = sparse_ref(dense_.back());

    // Swap the last entity with the one we're erasing. The one we're erasing is now at the back of the dense container.
    std::swap(dense_[*entity_index_ptr], dense_.back());
    // Update the index of the surviving entity with its new position
    last_entity_index = *entity_index_ptr;
    // and mark the removed entity as no longer existing.
    *entity_index_ptr = traits::tombstone;
    dense_.pop_back();
  }

  void erase(iterator start) { erase(*start); }

  void erase(iterator start, iterator end) {
    for (auto cur = start; start != end; ++start) {
      erase(*cur);
    }
  }

  iterator find(value_type entity) const {
    auto entity_index_ptr = sparse_ptr(entity);
    if (entity_index_ptr == nullptr || *entity_index_ptr == traits::tombstone) {
      return end();
    }

    return begin() + *entity_index_ptr;
  }

  uint32_t index(value_type entity) const { return sparse_ref(entity); }

  std::pair<iterator, bool> insert(value_type entity) {
    // Ensure we have a page allocated to store the entity index
    auto& sparse_index = grow_to_contain(entity);
    if (sparse_index != traits::tombstone) {
      return {dense_.begin() + sparse_index, false};
    }

    // The entity does not exist in the container, so we'll be adding it to the end of the dense container
    sparse_index = static_cast<uint32_t>(dense_.size());
    dense_.push_back(entity);

    return {dense_.begin() + sparse_index, true};
  }

  template <class TIt>
  void insert(TIt start, TIt end) {
    if constexpr (std::contiguous_iterator<TIt>) {
      reserve(static_cast<uint32_t>(end - start));
    }
    while (start != end) {
      insert(*start);
    }
  }

  void reserve(size_t count) { dense_.reserve(count); }

  size_t size() const { return dense_.size(); }

  value_type operator[](uint32_t index) const {
    assert(index < dense_.size());
    return dense_[index];
  }

 private:
  sparse_container sparse_;
  dense_container dense_;
};

using sparse_set = basic_sparse_set<entity>;
} // namespace ew