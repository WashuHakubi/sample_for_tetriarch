/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "engine/reflection/reflection.h"

namespace ewok {
std::unordered_map<std::string, ClassPtr>& Reflection::nameToClass() {
  static std::unordered_map<std::string, ClassPtr> nameToClass_;
  return nameToClass_;
}

std::unordered_map<std::type_index, ClassPtr>& Reflection::typeToClass() {
  static std::unordered_map<std::type_index, ClassPtr> typeToClass_;
  return typeToClass_;
};

auto Reflection::class_(std::string const& name) -> ClassPtr {
  auto it = nameToClass().find(name);
  return it == nameToClass().end() ? nullptr : it->second;
}
auto Reflection::class_(std::type_index type) -> ClassPtr {
  auto it = typeToClass().find(type);
  return it == typeToClass().end() ? nullptr : it->second;
}

Class::Class(
    std::string name,
    std::type_index type,
    std::vector<std::unique_ptr<Field>> fields)
    : MemberInfo(std::move(name), type) {
  fields_.reserve(fields.size());
  nameToField_.reserve(fields.size());

  for (auto&& f : fields) {
    auto p = f.release();
    fields_.push_back(p);
    nameToField_.emplace(p->name(), p);
  }
}

Class::~Class() {
  for (auto&& f : fields_) {
    delete f;
  }
}
} // namespace ewok
