/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace wut {
class game_object;
using game_object_ptr = std::shared_ptr<game_object>;
using game_object_handle = std::weak_ptr<game_object>;

class game_object : std::enable_shared_from_this<game_object> {
 public:
  game_object(std::string name);
  ~game_object();

 private:
  std::string name_;
  std::vector<game_object_ptr> children_;
  game_object* parent_;
};
} // namespace wut

namespace wutcs {
wut::game_object_ptr* create_game_object(char const* name);

void destroy_game_object(wut::game_object_ptr* handle);
} // namespace wutcs
