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
  // Earlier version
  setPacketHandlers(1, {});
  // Our version
  setPacketHandlers(2, {});
  // Next version
  setPacketHandlers(3, {});

  ProtocolVersion ours = {.version = 2};
  REQUIRE(ours.version == 2);

  REQUIRE(isCompatible(ours, ours));

  ProtocolVersion compatibleEarlier{.version = 1};
  REQUIRE(isCompatible(ours, compatibleEarlier));

  ProtocolVersion compatibleNewer{.version = 3};
  REQUIRE(isCompatible(ours, compatibleNewer));

  ProtocolVersion notCompatible{.version = 4};
  REQUIRE(!isCompatible(ours, notCompatible));
}

TEST_CASE("Can write entity packets") {
  std::string buffer;
  auto writer = ewok::shared::serialization::createBinWriter(buffer);

  TransformUpdate update = {
      .entityId = 1,
      .x = 2, .y = 3, .z = 4,
      .yaw = 5, .pitch = 6, .roll = 7
  };
  auto r = ewok::shared::serialization::serialize(*writer, update);
  REQUIRE(r.has_value());

  auto constexpr expectedSize =
      sizeof(TransformUpdate::entityId)
      + sizeof(TransformUpdate::x) + sizeof(TransformUpdate::y) + sizeof(TransformUpdate::z)
      + sizeof(TransformUpdate::yaw) + sizeof(TransformUpdate::pitch) + sizeof(TransformUpdate::roll);
  REQUIRE(writer->data().size() == expectedSize);
}