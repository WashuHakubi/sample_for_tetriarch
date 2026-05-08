/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include "game_object.h"

#include <spdlog/spdlog.h>

wut::game_object_ptr* wutcs::create_game_object(char const* name) {
  auto obj = std::make_shared<wut::game_object>(name ? name : "[game_object]");
  return new wut::game_object_ptr(obj);
}

void wutcs::destroy_game_object(wut::game_object_ptr* handle) {
  delete handle;
}

wut::game_object::game_object(std::string name) : name_(std::move(name)) {
  SPDLOG_INFO("Creating game object: {}", name_);
}

wut::game_object::~game_object() {
  SPDLOG_INFO("Destroying game object: {}", name_);
}
