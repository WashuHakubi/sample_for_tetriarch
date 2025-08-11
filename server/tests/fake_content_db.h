/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <shared/content_db.h>

namespace ewok {
struct FakeContentDb : shared::IContentDb {
  template <class T>
    requires(std::is_base_of_v<shared::ContentDef, T>)
  void registerItem(std::shared_ptr<T> p, shared::ContentScope scope) {
    auto id = p->id;
    db_.emplace(std::make_pair(id, std::type_index{typeid(T)}), p.get());

    const auto [items, _] = scopedContent_.emplace(
        std::make_pair(std::type_index{typeid(T)}, scope),
        std::vector<void const*>{});
    items->second.push_back(p.get());

    content_.push_back(p);
  }

  using IContentDb::get;

  using IContentDb::getAllInScope;

protected:
  auto get(xg::Guid const& id, std::type_index type) -> void const* override {
    if (auto const it = db_.find({id, type}); it != db_.end()) {
      return it->second;
    }

    LOG(FATAL) << "Could not find content item matching: " << id << " and " << type.name();
  }

  std::vector<void const*> getAllInScope(std::type_index type, shared::ContentScope scope) override {
    if (auto const it = scopedContent_.find({type, scope}); it != scopedContent_.end()) {
      return it->second;
    }

    return {};
  }

private:
  std::vector<std::shared_ptr<void>> content_;
  std::unordered_map<std::pair<xg::Guid, std::type_index>, void const*> db_;
  std::unordered_map<std::pair<std::type_index, shared::ContentScope>, std::vector<void const*>> scopedContent_;
};

void populateDb(FakeContentDb& contentDb);
}