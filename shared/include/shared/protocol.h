/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "shared/serialization.h"

namespace ewok::shared::protocol {
enum class PacketType : uint8_t {
  ProtocolVersion,
  Transform,
  TransformScale,

  CreateEntity,
  DestroyEntity,

  // Last entry, this is where packets not defined by the shared interface should start.
  Extended,
};

template <PacketType P>
struct Packet {
  PacketType type = P;
};

/// First packet that should be sent from the client and server. If the major version disagrees then the client or
/// server should be considered incompatible. If the minor version differs then the two should still be able to
/// communicate.
struct ProtocolVersion : Packet<PacketType::ProtocolVersion> {
  ProtocolVersion() = default;

  ProtocolVersion(uint32_t version)
    : version(version) {
  }

  uint32_t version;

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("type", &ProtocolVersion::type),
        std::make_pair("version", &ProtocolVersion::version));
  }
};

void initCompatTransforms(std::unordered_map<std::tuple<uint32_t, uint32_t>, std::function<void()>> transforms);
bool isCompatible(ProtocolVersion const& ours, ProtocolVersion const& theirs);

namespace v0 {
/// Updates the position and rotation of an entity. Scale is updated separately as it does not usually change.
struct TransformUpdate : Packet<PacketType::Transform> {
  uint32_t entityId;
  float x, y, z;
  float yaw, pitch, roll;

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("type", &TransformUpdate::type),
        std::make_pair("entityId", &TransformUpdate::entityId),
        std::make_pair("x", &TransformUpdate::x),
        std::make_pair("y", &TransformUpdate::y),
        std::make_pair("z", &TransformUpdate::z),
        std::make_pair("yaw", &TransformUpdate::yaw),
        std::make_pair("pitch", &TransformUpdate::pitch),
        std::make_pair("roll", &TransformUpdate::roll));
  }
};

/// Updates the scale of an entity. This is a separate packet as the scale of an entity does not normally change
/// frequently.
struct TransformScaleUpdate : Packet<PacketType::TransformScale> {
  uint32_t entityId;
  float scaleX, scaleY, scaleZ;

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("type", &TransformScaleUpdate::type),
        std::make_pair("entityId", &TransformScaleUpdate::entityId),
        std::make_pair("scaleX", &TransformScaleUpdate::scaleX),
        std::make_pair("scaleY", &TransformScaleUpdate::scaleY),
        std::make_pair("scaleZ", &TransformScaleUpdate::scaleZ));
  }
};

struct CreateEntity : Packet<PacketType::CreateEntity> {
  uint32_t entityId;
  std::string prefab;
  std::vector<std::pair<std::string, std::string>> attributes;

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("type", &CreateEntity::type),
        std::make_pair("entityId", &CreateEntity::entityId),
        std::make_pair("prefab", &CreateEntity::prefab),
        std::make_pair("attributes", &CreateEntity::attributes));
  }
};

struct DestroyEntity : Packet<PacketType::DestroyEntity> {
  uint32_t entityId;

  static constexpr auto serializeMembers() {
    return std::make_tuple(
        std::make_pair("type", &DestroyEntity::type),
        std::make_pair("entityId", &DestroyEntity::entityId));
  }
};
}

// Bring the correct versions into scope. We might have v1 instances of some packets later, and want to keep the v0 ones
// around to handle packets from those clients.
using TransformUpdate = v0::TransformUpdate;
using TransformScaleUpdate = v0::TransformScaleUpdate;
using CreateEntity = v0::CreateEntity;
using DestroyEntity = v0::DestroyEntity;
}