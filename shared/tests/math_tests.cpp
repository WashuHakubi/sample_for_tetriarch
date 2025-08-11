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
  glm::vec4 v4_1 = {float(M_1_PI), float(M_PI), float(M_E), 42.0f};
  glm::quat q1 = {1, 2, 3, 4};

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
  {
    writer->enter("quat");
    auto r = ewok::shared::serialization::serialize(*writer, q1);
    REQUIRE(r.has_value());
    writer->leave("quat");
  }

  auto data = writer->data();
  REQUIRE(
      data ==
      R"({"vec2":{"x":42.0,"y":84.0},"vec3":{"x":1.0,"y":2.0,"z":3.0},"vec4":{"x":0.31830987334251404,"y":3.1415927410125732,"z":2.7182817459106445,"w":42.0},"quat":{"x":2.0,"y":3.0,"z":4.0,"w":1.0}})")
  ;

  glm::vec2 v2_2;
  glm::vec3 v3_2;
  glm::vec4 v4_2;
  glm::quat q2;
  auto reader = ewok::shared::serialization::createJsonReader(data);
  {
    reader->enter("vec2");
    auto r = ewok::shared::serialization::deserialize(*reader, v2_2);
    REQUIRE(r.has_value());
    reader->leave("vec2");
  }
  {
    reader->enter("vec3");
    auto r = ewok::shared::serialization::deserialize(*reader, v3_2);
    REQUIRE(r.has_value());
    reader->leave("vec3");
  }
  {
    reader->enter("vec4");
    auto r = ewok::shared::serialization::deserialize(*reader, v4_2);
    REQUIRE(r.has_value());
    reader->leave("vec4");
  }
  {
    reader->enter("quat");
    auto r = ewok::shared::serialization::deserialize(*reader, q2);
    REQUIRE(r.has_value());
    reader->leave("quat");
  }

  REQUIRE(v2_1 == v2_2);
  REQUIRE(v3_1 == v3_2);
  REQUIRE(v4_1 == v4_2);
  REQUIRE(q1 == q2);
}
