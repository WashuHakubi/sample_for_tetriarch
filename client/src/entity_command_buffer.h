/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <vector>

#include "entity_db.h"

namespace ew {

template<class TEntity>
class basic_entity_command_buffer {
  using entity_db = basic_entity_db<TEntity>;
public:
  using entity_type = TEntity;

  basic_entity_command_buffer(basic_entity_db<entity_type>& db) : db_(&db) {}

  template<typename TComp>
  void assign(entity_type entity, TComp && comp) {
    auto fn = [c = std::move(comp)](entity_type e, entity_db& db) {
      db.assign(e, std::forward<TComp>(c));
    };
    commands_.emplace_back(entity, std::move(fn));
  }

  entity_type create() {
    return db_->create();
  }

  void destroy(entity_type entity) {
    auto fn = [](entity_type e, entity_db& db) {
      db.destroy(e);
    };
    commands_.emplace_back(entity, std::move(fn));
  }

  void execute() {
    for (auto&& [entity, fn] : commands_) {
      fn(entity, *db_);
    }
    commands_.clear();
  }

  template<class TComp>
  void remove(entity_type entity) {
    auto fn = [](entity_type e, entity_db& db) {
      db.template remove<TComp>(e);
    };
    commands_.emplace_back(entity, std::move(fn));
  }
private:
  basic_entity_db<entity_type>* db_;
  std::vector<std::pair<entity_type, std::function<void(entity_type, entity_db&)>>> commands_;
};

using entity_command_buffer = basic_entity_command_buffer<entity>;
}