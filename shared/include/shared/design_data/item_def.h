/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

namespace ewok::shared::design_data {
struct ItemDef : ContentDef {
  std::string name;

  static constexpr auto serializationMembers() {
    return std::make_tuple(
        std::make_pair("id", &ItemDef::id),
        std::make_pair("name", &ItemDef::name));
  }
};
}