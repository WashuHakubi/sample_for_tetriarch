/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once
#include <vector>

#include "sparse_set.h"

namespace ew {
template <class TEntity, class TComp>
class basic_table {
 public:
  using entity_type = TEntity;
  using value_type = TComp;
  using iterator = std::vector<value_type>::iterator;
  using const_iterator = std::vector<value_type>::const_iterator;

  iterator begin() { return components_.begin(); }

  const_iterator begin() const { return components_.begin(); }

  void clear() {
    entities_.clear();
    components_.clear();
  }

  bool contains(entity_type entity) const { return entities_.contains(entity); }

  iterator end() { return components_.end(); }

  const_iterator end() const { return components_.end(); }

  auto& entities() const { return entities_; }

  void erase(entity_type entity) {
    auto it = entities_.find(entity);
    if (it != entities_.end()) {
      components_.erase(components_.begin() + (it - entities_.begin()));
      entities_.erase(it);
    }
  }

  std::pair<iterator, bool> insert(entity_type entity, value_type const& comp) {
    auto [it, inserted] = entities_.insert(entity);
    if (inserted) {
      components_.push_back(comp);
      return {components_.begin() + components_.size() - 1, true};
    }

    return {components_.begin() + (it - entities_.begin()), false};
  }

  std::pair<iterator, bool> insert_or_assign(entity_type entity, value_type const& comp) {
    auto [it, inserted] = entities_.insert(entity);
    if (inserted) {
      components_.push_back(comp);
      return {components_.begin() + components_.size() - 1, true};
    }

    components_[(it - entities_.begin())] = comp;
    return {components_.begin() + components_.size() - 1, false};
  }

  std::pair<iterator, bool> insert(entity_type entity, value_type&& comp) {
    auto [it, inserted] = entities_.insert(entity);
    if (inserted) {
      components_.push_back(std::forward<value_type>(comp));
      return {components_.begin() + components_.size() - 1, true};
    }

    return {components_.begin() + (it - entities_.begin()), false};
  }

  std::pair<iterator, bool> insert_or_assign(entity_type entity, value_type&& comp) {
    auto [it, inserted] = entities_.insert(entity);
    if (inserted) {
      components_.push_back(std::forward<value_type>(comp));
      return {components_.begin() + components_.size() - 1, true};
    }

    components_[(it - entities_.begin())] = std::forward<value_type>(comp);
    return {components_.begin() + components_.size() - 1, false};
  }

  void reserve(size_t count) {
    entities_.reserve(count);
    components_.reserve(count);
  }

  size_t size() { return components_.size(); }

  value_type const& value(entity_type entity) const {
    auto idx = entities_.index(entity);
    assert(idx < components_.size());
    return components_[idx];
  }

  value_type& value(entity_type entity) {
    auto idx = entities_.index(entity);
    assert(idx < components_.size());
    return components_[idx];
  }

 private:
  basic_sparse_set<entity_type> entities_;
  std::vector<value_type> components_;
};

template <class TComp>
using table = basic_table<entity, TComp>;
} // namespace ew