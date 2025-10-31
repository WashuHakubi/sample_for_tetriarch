/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once
#include <functional>

#include "sparse_set.h"
#include "table.h"
#include "tuple_traits.h"

namespace ew {
template <class TEntity, class... TArgs>
class basic_query {
 public:
  using tables_tuple = std::tuple<basic_table<TEntity, std::decay_t<TArgs>>*...>;

  explicit basic_query(tables_tuple tables) : tables_(std::move(tables)) {}

  void visit(std::function<void(std::add_lvalue_reference_t<TArgs>...)> const& fn) {
    for (auto& entities = smallest_entities_set(); auto&& entity : entities) {
      if (!all_contain(entity)) {
        continue;
      }

      fn(get<TArgs>(entity)...);
    }
  }

  void visit(std::function<void(TEntity, std::add_lvalue_reference_t<TArgs>...)> const& fn) {
    for (auto& entities = smallest_entities_set(); auto&& entity : entities) {
      if (!all_contain(entity)) {
        continue;
      }

      fn(entity, get<TArgs>(entity)...);
    }
  }

 private:
  basic_sparse_set<TEntity> const& smallest_entities_set() const {
    basic_sparse_set<TEntity> const* entities {nullptr};
    visit_each(tables_, [&entities](auto const& table) {
      if (entities == nullptr || entities->size() > table->size()) {
        entities = &table->entities();
      }
    });

    assert(entities != nullptr);
    return *entities;
  }

  bool all_contain(TEntity entity) const {
    bool missing = false;
    visit_each(tables_, [entity, &missing](auto const& table) {
      if (!table->contains(entity)) {
        missing = true;
      }
    });

    return !missing;
  }

  template <class TArg>
  auto& get(TEntity entity) {
    using table_entry = basic_table<TEntity, std::decay_t<TArg>>*;
    auto& table = std::get<table_entry>(tables_);
    return table->value(entity);
  }

  tables_tuple tables_;
};

template <class... TComps>
using query = basic_query<entity, TComps...>;
} // namespace ew