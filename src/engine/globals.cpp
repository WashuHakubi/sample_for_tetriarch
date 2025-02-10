/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include "engine/asset_database.h"

#include <cassert>

namespace ewok {
namespace {
std::shared_ptr<AssetDatabase> s_assetDatabase;
std::shared_ptr<concurrencpp::executor> s_globalExecutor;
std::shared_ptr<ObjectDatabase> s_objectDatabase;
} // namespace

std::shared_ptr<AssetDatabase> const& assetDatabase() {
  assert(s_assetDatabase != nullptr);
  return s_assetDatabase;
}

std::shared_ptr<concurrencpp::executor> const& globalExecutor() {
  assert(s_globalExecutor != nullptr);
  return s_globalExecutor;
}

std::shared_ptr<ObjectDatabase> const& objectDatabase() {
  assert(s_objectDatabase != nullptr);
  return s_objectDatabase;
}

void setAssetDatabase(std::shared_ptr<AssetDatabase> db) {
  assert(s_assetDatabase == nullptr);
  s_assetDatabase = std::move(db);
}

void setGlobalExecutor(std::shared_ptr<concurrencpp::executor> executor) {
  assert(s_globalExecutor == nullptr);
  s_globalExecutor = std::move(executor);
}

void setObjectDatabase(std::shared_ptr<ObjectDatabase> db) {
  assert(s_objectDatabase == nullptr);
  s_objectDatabase = std::move(db);
}
} // namespace ewok
