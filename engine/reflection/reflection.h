/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <cassert>
#include <memory>
#include <span>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ewok {
class Field;

class TypeBase {
 public:
  TypeBase(std::string name, std::type_index type)
      : name_(std::move(name)), type_(type) {}

  virtual ~TypeBase() = default;

  auto name() const -> std::string const& { return name_; }
  auto type() const -> std::type_index const& { return type_; }

 private:
  std::string name_;
  std::type_index type_;
};

class Class : public TypeBase {
 public:
  using TypeBase::TypeBase;

  auto fields() const -> std::span<std::unique_ptr<Field> const> {
    return fields_;
  }

  template <class C, class F>
  auto field(F C::*m, std::string name) -> Class&;

 private:
  std::vector<std::unique_ptr<Field>> fields_;
  std::unordered_map<std::string, size_t> nameToFieldIndex_;
};

class Reflection {
 public:
  template <class T>
  static Class& class_(std::string name) {
    auto p = std::make_unique<Class>(name, typeid(T));
    auto resp = p.get();
    typeToClass_.emplace(typeid(T), p.get());
    nameToClass_.emplace(std::move(name), std::move(p));
    return *resp;
  }

  static auto getClass(std::string const& name) -> Class const* {
    auto it = nameToClass_.find(name);
    return it != nameToClass_.end() ? it->second.get() : nullptr;
  }

  static auto getClass(std::type_index type) -> Class const* {
    auto it = typeToClass_.find(type);
    return it != typeToClass_.end() ? it->second : nullptr;
  }

  template <class T>
  static auto getClass() -> Class const* {
    return getClass(typeid(T));
  }

 private:
  inline static std::unordered_map<std::type_index, Class*> typeToClass_{};
  inline static std::unordered_map<std::string, std::unique_ptr<Class>>
      nameToClass_{};
};

class Field : public TypeBase {
 public:
  using TypeBase::TypeBase;

  auto getClass() const -> Class const* { return Reflection::getClass(type()); }

  virtual void setValue(
      void* instance, std::type_index srcType, void const* srcValue) = 0;

  virtual void* getValue(void* instance) const = 0;
};

template <class C, class F>
class TypedField : public Field {
 public:
  TypedField(std::string name, F C::*field)
      : Field(std::move(name), typeid(F)), field_(field) {}

  void setValue(
      void* instance, std::type_index srcType, void const* srcValue) override {
    assert(instance);
    assert(srcType == type());
    auto cp = static_cast<C*>(instance);
    auto fp = static_cast<F const*>(srcValue);
    (cp->*field_) = *fp;
  }

  auto getValue(void* instance) const -> void* override {
    assert(instance);
    auto cp = static_cast<C*>(instance);
    return &(cp->*field_);
  }

 private:
  F C::*field_;
};

template <class C, class F>
auto Class::field(F C::*m, std::string name) -> Class& {
  fields_.push_back(std::make_unique<TypedField<C, F>>(name, m));
  nameToFieldIndex_.emplace(std::move(name), fields_.size() - 1);
  return *this;
}
} // namespace ewok

#define EWOK_CONCAT(a, b) EWOK_CONCAT_INNER(a, b)
#define EWOK_CONCAT_INNER(a, b) a##b
#define EWOK_UNIQUE_NAME(base) EWOK_CONCAT(base, __COUNTER__)

#define EWOK_REGISTRATION                                           \
  static void ewok_auto_register_reflection_function_();            \
  namespace {                                                       \
  struct ewok_auto_register_ {                                      \
    ewok_auto_register_() {                                         \
      ewok_auto_register_reflection_function_();                    \
    }                                                               \
  };                                                                \
  }                                                                 \
  static const ewok_auto_register_ EWOK_UNIQUE_NAME(autoRegister_); \
  static void ewok_auto_register_reflection_function_()

#define EWOK_REFLECTION_DECL \
  friend void ewok_auto_register_reflection_function_();