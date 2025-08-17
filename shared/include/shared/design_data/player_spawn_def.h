/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

namespace ewok::shared::design_data {
struct player_spawn_def {
  glm::vec3 position;
  glm::vec3 facing;

  static constexpr auto serializationMembers() {
    return std::make_tuple(
        std::make_pair("position", &player_spawn_def::position),
        std::make_pair("facing", &player_spawn_def::facing));
  }
};
}