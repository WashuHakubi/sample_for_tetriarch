#include "engine/asset_database.h"

#include <cassert>

namespace ewok {
namespace {
std::shared_ptr<AssetDatabase> s_assetDatabase;
std::shared_ptr<concurrencpp::executor> s_globalExecutor;
} // namespace

std::shared_ptr<AssetDatabase> const& assetDatabase() {
  assert(s_assetDatabase != nullptr);
  return s_assetDatabase;
}

std::shared_ptr<concurrencpp::executor> const& globalExecutor() {
  assert(s_globalExecutor != nullptr);
  return s_globalExecutor;
}

void setAssetDatabase(std::shared_ptr<AssetDatabase> db) {
  assert(s_assetDatabase == nullptr);
  s_assetDatabase = db;
}

void setGlobalExecutor(std::shared_ptr<concurrencpp::executor> executor) {
  assert(s_globalExecutor == nullptr);
  s_globalExecutor = executor;
}
} // namespace ewok
