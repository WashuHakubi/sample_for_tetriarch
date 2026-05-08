/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <string>
#include <vector>

#include "ref_ptr.h"

namespace wut {
class game_object;
using game_object_ptr = ref_ptr<game_object>;

class game_object : public ref_count_mixin<game_object> {
 public:
  game_object(std::string name);
  ~game_object();

  auto name() const noexcept -> std::string const& { return name_; }

  auto parent() const noexcept -> game_object_ptr { return {parent_}; }

 private:
  std::string name_;
  std::vector<game_object_ptr> children_;
  game_object* parent_;
};
} // namespace wut

namespace wutcs {
wut::game_object* create_game_object(char const* name);

void release_game_object(wut::game_object* handle);

void acquire_game_object(wut::game_object* handle);

char const* game_object_name(wut::game_object* handle);
} // namespace wutcs
