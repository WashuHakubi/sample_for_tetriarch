/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shared/math.h"
#include <catch2/catch_all.hpp>

TEST_CASE("Can serialize/deserialize vectors") {
  glm::vec2 v2_1 = {42.0f, 84.0f};
  glm::vec3 v3_1 = {1, 2, 3};
  glm::vec4 v4_1 = {M_1_PIf, M_PIf, M_Ef, 42.0f};

  auto writer = ewok::shared::serialization::createJsonWriter();
  {
    writer->enter("vec2");
    auto r = ewok::shared::serialization::serialize(*writer, v2_1);
    REQUIRE(r.has_value());
    writer->leave("vec2");
  }
  {
    writer->enter("vec3");
    auto r = ewok::shared::serialization::serialize(*writer, v3_1);
    REQUIRE(r.has_value());
    writer->leave("vec3");
  }
  {
    writer->enter("vec4");
    auto r = ewok::shared::serialization::serialize(*writer, v4_1);
    REQUIRE(r.has_value());
    writer->leave("vec4");
  }

  auto data = writer->data();
  REQUIRE(
      data ==
      R"({"vec2":{"x":42.0,"y":84.0},"vec3":{"x":1.0,"y":2.0,"z":3.0},"vec4":{"x":0.31830987334251404,"y":3.1415927410125732,"z":2.7182817459106445,"w":42.0}})")
  ;
}
