/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shared/protocol.h"

#include "catch2/catch_all.hpp"

using namespace ewok::shared::protocol;

TEST_CASE("Can check versions") {
  initCompatTransforms(
  {
      // Our version (2) is compatible with an earlier version (1)
      {{2, 1}, [] {
      }},

      // Our version (2) is compatible with a newer version (3)
      {{2, 3}, [] {
      }},
  });

  ProtocolVersion ours = {2};
  REQUIRE(ours.type == PacketType::ProtocolVersion);
  REQUIRE(ours.version == 2);

  REQUIRE(isCompatible(ours, ours));

  ProtocolVersion compatibleEarlier{1};
  REQUIRE(isCompatible(ours, compatibleEarlier));

  ProtocolVersion compatibleNewer{3};
  REQUIRE(isCompatible(ours, compatibleNewer));

  ProtocolVersion notCompatible{4};
  REQUIRE(!isCompatible(ours, notCompatible));
}

TEST_CASE("Can write entity packets") {
  auto writer = ewok::shared::serialization::createBinWriter();

  TransformUpdate update = {PacketType::Transform, 2, 3, 4, 5, 6, 7, 8};
  auto r = ewok::shared::serialization::serialize(*writer, update);
  REQUIRE(r.has_value());

  auto constexpr expectedSize = sizeof(PacketType)
      + sizeof(TransformUpdate::entityId)
      + sizeof(TransformUpdate::x) + sizeof(TransformUpdate::y) + sizeof(TransformUpdate::z)
      + sizeof(TransformUpdate::yaw) + sizeof(TransformUpdate::pitch) + sizeof(TransformUpdate::roll);
  REQUIRE(writer->data().size() == expectedSize);
}