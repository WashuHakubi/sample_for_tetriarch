/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>

#include "archetypes.h"

struct Transform {};

struct NpcCharacter {
  std::string name;
};

struct Hostile {};

int main() {
  ew::Archetypes arches;
  // arches.registerComponent<Transform>();
  // arches.registerComponent<NpcCharacter>();
  // arches.registerComponent<Hostile>();

  [[maybe_unused]] auto friendly = arches.create<Transform, NpcCharacter>({}, {"Johnny Bravo"});

  [[maybe_unused]] auto unfriendly = arches.create<Transform, NpcCharacter, Hostile>();
  [[maybe_unused]] auto unfriendly2 = arches.create<Transform, NpcCharacter, Hostile>();

  std::cout << "All entities" << std::endl;
  arches.query().forEach([](ew::Entity const e) {
    std::cout << e.descriptor->id << " " << e.descriptor->archetype << std::endl;
  });

  std::cout << "Friendlies in cherno:" << std::endl;
  arches.query().without<Hostile>().forEach([](ew::Entity const e, NpcCharacter const& npc) {
    std::cout << e.descriptor->id << " " << e.descriptor->archetype << ": " << npc.name << std::endl;
  });

  std::cout << "Hostile entities" << std::endl;
  arches.query().forEach([](ew::Entity const e, Hostile const&) {
    std::cout << e.descriptor->id << " " << e.descriptor->archetype << std::endl;
  });
}
