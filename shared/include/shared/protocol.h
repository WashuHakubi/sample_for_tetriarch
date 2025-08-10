/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "shared/serialization.h"

namespace ewok::shared::protocol {
enum class PacketType : uint16_t {
  ProtocolVersion,

  Transform,
  TransformScale,

  CreateEntity,
  DestroyEntity,

  // Last entry, this is where packets not defined by the shared interface should start.
  Extended,
};

template <PacketType V>
struct Packet {
  static constexpr uint16_t PacketType = static_cast<uint16_t>(V);

  bool operator==(Packet const&) const = default;
};

/// First packet that should be sent from the client and server. If the major version disagrees then the client or
/// server should be considered incompatible. If the minor version differs then the two should still be able to
/// communicate.
struct ProtocolVersion : Packet<PacketType::ProtocolVersion> {
  uint32_t version;

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("version", &ProtocolVersion::version));
  }

  bool operator==(ProtocolVersion const&) const = default;
};

bool isCompatible(ProtocolVersion const& ours, ProtocolVersion const& theirs);

auto getPacketHandler(
    uint32_t version,
    PacketType type) -> std::function<serialization::Result(serialization::IReader& reader)> const&;

void setPacketHandlers(
    uint32_t version,
    std::vector<std::function<serialization::Result(serialization::IReader& reader)>> handlers);

/// Dispatches to the appropriate packet handler for the packet type and protocol version.
/// The type is expected to be the first short read from the buffer, followed by the packet data.
auto dispatchPacket(uint32_t version, serialization::IReader& reader) -> serialization::Result;


auto writePacket(serialization::IWriter& writer, auto packet) -> serialization::Result {
  auto type = std::remove_cvref_t<decltype(packet)>::PacketType;
  if (auto r = serialization::detail::serializeItem(writer, "$type", type); !r) {
    return r;
  }

  return serialization::serialize(writer, packet);
}

namespace v0 {
/// Updates the position and rotation of an entity. Scale is updated separately as it does not usually change.
struct TransformUpdate : Packet<PacketType::Transform> {
  uint32_t entityId;
  float x, y, z;
  float yaw, pitch, roll;

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("entityId", &TransformUpdate::entityId),
        std::make_pair("x", &TransformUpdate::x),
        std::make_pair("y", &TransformUpdate::y),
        std::make_pair("z", &TransformUpdate::z),
        std::make_pair("yaw", &TransformUpdate::yaw),
        std::make_pair("pitch", &TransformUpdate::pitch),
        std::make_pair("roll", &TransformUpdate::roll));
  }

  bool operator==(const TransformUpdate&) const = default;
};

/// Updates the scale of an entity. This is a separate packet as the scale of an entity does not normally change
/// frequently.
struct TransformScaleUpdate : Packet<PacketType::TransformScale> {
  uint32_t entityId;
  float scaleX, scaleY, scaleZ;

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("entityId", &TransformScaleUpdate::entityId),
        std::make_pair("scaleX", &TransformScaleUpdate::scaleX),
        std::make_pair("scaleY", &TransformScaleUpdate::scaleY),
        std::make_pair("scaleZ", &TransformScaleUpdate::scaleZ));
  }

  bool operator==(const TransformScaleUpdate&) const = default;
};

struct CreateEntity : Packet<PacketType::CreateEntity> {
  uint32_t entityId;
  std::string prefab;
  std::vector<std::pair<std::string, std::string>> attributes;

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("entityId", &CreateEntity::entityId),
        std::make_pair("prefab", &CreateEntity::prefab),
        std::make_pair("attributes", &CreateEntity::attributes));
  }

  bool operator==(const CreateEntity&) const = default;
};

struct DestroyEntity : Packet<PacketType::DestroyEntity> {
  uint32_t entityId;

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("entityId", &DestroyEntity::entityId));
  }

  bool operator==(const DestroyEntity&) const = default;
};
}

// Bring the correct versions into scope. We might have v1 instances of some packets later, and want to keep the v0 ones
// around to handle packets from those clients.
using TransformUpdate = v0::TransformUpdate;
using TransformScaleUpdate = v0::TransformScaleUpdate;
using CreateEntity = v0::CreateEntity;
using DestroyEntity = v0::DestroyEntity;
}