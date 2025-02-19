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
}

std::unordered_map<std::string, EnumPtr>& Reflection::nameToEnum() {
  static std::unordered_map<std::string, EnumPtr> nameToEnum_;
  return nameToEnum_;
}

std::unordered_map<std::type_index, EnumPtr>& Reflection::typeToEnum() {
  static std::unordered_map<std::type_index, EnumPtr> typeToEnum_;
  return typeToEnum_;
}

auto Reflection::class_(std::string const& name) -> ClassPtr {
  auto it = nameToClass().find(name);
  return it == nameToClass().end() ? nullptr : it->second;
}
auto Reflection::class_(std::type_index type) -> ClassPtr {
  auto it = typeToClass().find(type);
  return it == typeToClass().end() ? nullptr : it->second;
}

auto Reflection::enum_(std::string const& name) -> EnumPtr {
  auto it = nameToEnum().find(name);
  return it == nameToEnum().end() ? nullptr : it->second;
}
auto Reflection::enum_(std::type_index type) -> EnumPtr {
  auto it = typeToEnum().find(type);
  return it == typeToEnum().end() ? nullptr : it->second;
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

Enum::Enum(
    std::string name,
    std::type_index type,
    std::vector<std::pair<size_t, std::string>> valueNames)
    : MemberInfo(std::move(name), type), valueNames_(std::move(valueNames)) {
  for (auto&& [value, name] : valueNames_) {
    valueToName_.emplace(value, name);
    nameToValue_.emplace(name, value);
  }
}

auto Enum::name(size_t val) const -> std::optional<std::string> {
  if (auto it = valueToName_.find(val); it != valueToName_.end()) {
    return it->second;
  }
  return std::nullopt;
}

auto Enum::value(std::string const& name) const -> std::optional<size_t> {
  if (auto it = nameToValue_.find(name); it != nameToValue_.end()) {
    return it->second;
  }
  return std::nullopt;
}
} // namespace ewok
