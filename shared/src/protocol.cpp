/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shared/protocol.h"

namespace ewok::shared::protocol {
static std::unordered_map<std::tuple<uint32_t, uint32_t>, std::function<void()>> s_fromToVersionTransforms;

void initCompatTransforms(std::unordered_map<std::tuple<uint32_t, uint32_t>, std::function<void()>> transforms) {
  s_fromToVersionTransforms = std::move(transforms);
}

bool isCompatible(ProtocolVersion const& ours, ProtocolVersion const& theirs) {
  if (ours.version == theirs.version) {
    return true;
  }
  if (auto it = s_fromToVersionTransforms.find({ours.version, theirs.version}); it != s_fromToVersionTransforms.end()) {
    return true;
  }

  return false;
}
}