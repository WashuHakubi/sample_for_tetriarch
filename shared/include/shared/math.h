/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "shared/serialization.h"

namespace ewok::shared::serialization {
template <typename T, glm::qualifier Q>
struct SerializeMembers<glm::vec<2, T, Q>> : std::true_type {
  static auto getSerializeMembers() {
    return std::make_tuple(
        std::make_pair("x", &glm::vec<2, T, Q>::x),
        std::make_pair("y", &glm::vec<2, T, Q>::y));
  }
};

template <typename T, glm::qualifier Q>
struct SerializeMembers<glm::vec<3, T, Q>> : std::true_type {
  static auto getSerializeMembers() {
    return std::make_tuple(
        std::make_pair("x", &glm::vec<3, T, Q>::x),
        std::make_pair("y", &glm::vec<3, T, Q>::y),
        std::make_pair("z", &glm::vec<3, T, Q>::z));
  }
};

template <typename T, glm::qualifier Q>
struct SerializeMembers<glm::vec<4, T, Q>> : std::true_type {
  static auto getSerializeMembers() {
    return std::make_tuple(
        std::make_pair("x", &glm::vec<4, T, Q>::x),
        std::make_pair("y", &glm::vec<4, T, Q>::y),
        std::make_pair("z", &glm::vec<4, T, Q>::z),
        std::make_pair("w", &glm::vec<4, T, Q>::w));
  }
};
}