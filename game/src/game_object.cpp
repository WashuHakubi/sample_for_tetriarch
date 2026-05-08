/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include "game_object.h"

#include <spdlog/spdlog.h>

wut::game_object* wutcs::create_game_object(char const* name) {
  auto obj = wut::make_ref_counted<wut::game_object>(name);
  return obj.release();
}

void wutcs::release_game_object(wut::game_object* handle) {
  if (!handle)
    return;
  handle->release();
}

void wutcs::acquire_game_object(wut::game_object* handle) {
  if (handle) {
    handle->add_ref();
  }
}

char const* wutcs::game_object_name(wut::game_object* handle) {
  if (handle) {
    return handle->name().c_str();
  }
  return nullptr;
}

wut::game_object::game_object(std::string name) : name_(std::move(name)) {
  SPDLOG_INFO("Creating game object: {}", name_);
}

wut::game_object::~game_object() {
  SPDLOG_INFO("Destroying game object: {}", name_);
}
