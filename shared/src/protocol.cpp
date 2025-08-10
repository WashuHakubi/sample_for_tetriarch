/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shared/protocol.h"

namespace ewok::shared::protocol {
std::vector<std::vector<std::function<serialization::Result(serialization::IReader& reader)>>>
s_versionToPacketTypeToHandler;

void setPacketHandlers(
    uint32_t version,
    std::vector<std::function<serialization::Result(serialization::IReader& reader)>> handlers) {
  if (s_versionToPacketTypeToHandler.size() <= version) {
    s_versionToPacketTypeToHandler.resize(version + 1);
  }

  s_versionToPacketTypeToHandler[version] = std::move(handlers);
}

auto getPacketHandler(
    uint32_t version,
    PacketType type) -> std::function<serialization::Result(serialization::IReader& reader)> const& {
  assert(s_versionToPacketTypeToHandler.size() > version);
  auto const& handlers = s_versionToPacketTypeToHandler[version];

  assert(handlers.size() > static_cast<uint16_t>(type));
  return handlers[static_cast<uint16_t>(type)];
}

bool isCompatible(ProtocolVersion const& ours, ProtocolVersion const& theirs) {
  if (ours.version == theirs.version) {
    return true;
  }

  if (s_versionToPacketTypeToHandler.size() <= theirs.version) {
    return false;
  }

  return s_versionToPacketTypeToHandler[theirs.version].empty();
}

auto dispatchPacket(uint32_t version, serialization::IReader& reader) -> serialization::Result {
  PacketType type;
  if (auto r = serialization::detail::deserializeItem(reader, "type", type); !r) {
    return r;
  }

  if (auto const& dispatcher = getPacketHandler(version, type)) {
    return dispatcher(reader);
  }
  return {};
}
}