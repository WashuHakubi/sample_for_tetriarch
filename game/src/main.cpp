/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <coro/task.hpp>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <nethost.h>

using namespace std::string_view_literals;

int main(int argc, char** argv) {
  // auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  spdlog::info("asdf");
}
