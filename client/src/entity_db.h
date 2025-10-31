/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

#include "entity_traits.h"
#include "query.h"
#include "table.h"

namespace ew {
template <class TEntity>
class basic_entity_db {
 public:
  using entity_type = TEntity;

 private:
  using traits = entity_traits<TEntity>;

  struct abstract_table_container {
    virtual ~abstract_table_container() = default;

    virtual void erase(TEntity entity) = 0;
  };

  template <class TComp>
  struct table_container final : abstract_table_container {
    void erase(TEntity entity) override { table.erase(entity); }

    basic_table<TEntity, TComp> table;
  };

 public:
  template <class... TComps>
  void assign(entity_type entity, TComps&&... comps) {
    auto entity_it = entity_components_.find(entity);
    assert(entity_it != entity_components_.end());

    (assign_impl(entity, entity_it->second, std::forward<TComps>(comps)), ...);
  }

  [[nodiscard]] entity_type create() {
    if (!free_entities_.empty()) {
      auto ent = free_entities_.back();
      free_entities_.pop_back();
      entity_components_.emplace(ent, std::unordered_set<std::type_index>{});
      return ent;
    }

    auto ent = next_entity_;
    next_entity_ = traits::next_entity(next_entity_);
    entity_components_.emplace(ent, std::unordered_set<std::type_index>{});
    return ent;
  }

  void destroy(entity_type entity) {
    auto entity_it = entity_components_.find(entity);
    assert(entity_it != entity_components_.end());

    for (auto&& type : entity_it->second) {
      auto const& table = tables_.find(type)->second;
      table->erase(entity);
    }

    entity_components_.erase(entity_it);
    free_entities_.push_back(entity);
  }

  template <class... TComps>
  [[nodiscard]] basic_query<entity_type, TComps...> query() const {
    return basic_query<entity_type, TComps...>{build_tuple<TComps...>()};
  }

  template <class... TComps>
  void remove(entity_type entity) {
    auto entity_it = entity_components_.find(entity);
    assert(entity_it != entity_components_.end());

    (remove_impl<TComps>(entity, entity_it->second), ...);
  }

 private:
  template <class TComp>
  void assign_impl(entity_type entity, std::unordered_set<std::type_index>& components, TComp&& comp) {
    using component_type = std::decay_t<TComp>;
    auto table_it = tables_.find(typeid(component_type));
    if (table_it == tables_.end()) {
      std::cout << "A:" << typeid(component_type).name() << std::endl;
      table_it = tables_.emplace(typeid(component_type), std::make_unique<table_container<component_type>>()).first;
    }

    auto& table = static_cast<table_container<component_type>*>(table_it->second.get())->table;
    table.insert_or_assign(entity, std::forward<TComp>(comp));
    components.insert(typeid(component_type));
  }

  template <class TComp>
  void remove_impl(entity_type entity, std::unordered_set<std::type_index>& components) {
    using component_type = std::decay_t<TComp>;
    auto table_it = tables_.find(typeid(component_type));
    if (table_it == tables_.end()) {
      return;
    }

    auto& table = static_cast<table_container<component_type>*>(table_it->second.get())->table;
    table.erase(entity);
    components.erase(typeid(component_type));
  }

  template <class... TComps>
  [[nodiscard]] std::tuple<basic_table<entity_type, TComps>*...> build_tuple() const {
    return std::make_tuple(get_table<TComps>()...);
  }

  template <class TComp>
  [[nodiscard]] basic_table<entity_type, TComp>* get_table() const {
    using component_type = std::decay_t<TComp>;
    std::cout << "G:" << typeid(component_type).name() << std::endl;
    auto table_it = tables_.find(typeid(component_type));
    assert(table_it != tables_.end());
    return &static_cast<table_container<component_type>*>(table_it->second.get())->table;
  }

  entity_type next_entity_{};
  std::vector<entity_type> free_entities_;
  std::unordered_map<entity_type, std::unordered_set<std::type_index>> entity_components_;
  std::unordered_map<std::type_index, std::unique_ptr<abstract_table_container>> tables_;
};

using entity_db = basic_entity_db<entity>;
} // namespace ew
