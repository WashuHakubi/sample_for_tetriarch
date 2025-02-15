/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <cassert>
#include <format>
#include <memory>
#include <span>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ewok {
class Class;
using ClassPtr = std::shared_ptr<Class>;
class Field;
using FieldPtr = std::shared_ptr<Field>;
class FieldArrayAdapter;
using FieldArrayAdapterPtr = std::shared_ptr<FieldArrayAdapter>;

class TypeBase {
 public:
  TypeBase(std::string name, std::type_index type)
      : name_(std::move(name)), type_(type) {}

  virtual ~TypeBase() = default;

  virtual auto name() const -> std::string const& { return name_; }
  virtual auto type() const -> std::type_index const& { return type_; }

 private:
  std::string name_;
  std::type_index type_;
};

class Field : public TypeBase {
 public:
  using TypeBase::TypeBase;

  auto getClass() const -> ClassPtr;

  virtual void setValue(
      void* instance, std::type_index srcType, void const* srcValue) = 0;

  template <class T>
  void setValue(void* instance, T const& value) {
    setValue(instance, typeid(T), &value);
  }

  virtual auto getValue(void* instance, std::type_index expected) const
      -> void* = 0;

  template <class T>
  auto getValue(void* instance) const -> T {
    return *reinterpret_cast<T*>(getValue(instance, typeid(T)));
  }

  virtual auto getArrayAdapter() const -> FieldArrayAdapterPtr = 0;
};

class Class : public TypeBase {
 public:
  using TypeBase::TypeBase;

  auto fields() const -> std::span<FieldPtr const> { return fields_; }

  template <class C, class F>
  auto field(F C::*m, std::string name) -> Class&;

 private:
  std::vector<FieldPtr> fields_;
  std::unordered_map<std::string, FieldPtr> nameToField_;
};

class Reflection {
 public:
  template <class T>
  static Class& class_(std::string name) {
    auto p = std::make_shared<Class>(name, typeid(T));
    auto resp = p.get();
    typeToClass_.emplace(typeid(T), p);
    nameToClass_.emplace(std::move(name), std::move(p));
    return *resp;
  }

  static auto getClass(std::string const& name) -> ClassPtr {
    auto it = nameToClass_.find(name);
    return it != nameToClass_.end() ? it->second : nullptr;
  }

  static auto getClass(std::type_index type) -> ClassPtr {
    auto it = typeToClass_.find(type);
    return it != typeToClass_.end() ? it->second : nullptr;
  }

  template <class T>
  static auto getClass() -> ClassPtr {
    return getClass(typeid(T));
  }

 private:
  inline static std::unordered_map<std::type_index, ClassPtr> typeToClass_{};
  inline static std::unordered_map<std::string, ClassPtr> nameToClass_{};
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

  auto getValue(void* instance, std::type_index expected) const
      -> void* override {
    assert(instance);
    assert(expected == type());
    auto cp = static_cast<C*>(instance);
    return &(cp->*field_);
  }

  auto getArrayAdapter() const -> FieldArrayAdapterPtr override {
    return nullptr;
  }

 private:
  F C::*field_;
};

struct SyntheticArrayField : Field {
  using Field::Field;

  void setIndex(size_t index) {
    index_ = index;
    name_ = std::format("{}", index_);
  }

  auto name() const -> std::string const& override { return name_; }

 protected:
  size_t index_{};
  std::string name_;
};

template <class T>
struct TSyntheticArrayField : SyntheticArrayField {
  TSyntheticArrayField() : SyntheticArrayField("__array_item__", typeid(T)) {}

  void setValue(
      void* instance, std::type_index srcType, void const* srcValue) override {
    assert(instance);
    assert(srcType == type());

    auto cp = static_cast<std::vector<T>*>(instance);
    auto fp = static_cast<T const*>(srcValue);
    (*cp)[index_] = *fp;
  }

  auto getValue(void* instance, std::type_index expected) const
      -> void* override {
    assert(instance);
    assert(expected == type());

    auto cp = static_cast<std::vector<T>*>(instance);
    return &(*cp)[index_];
  }

  auto getArrayAdapter() const -> FieldArrayAdapterPtr override {
    return nullptr;
  }
};

struct FieldArrayAdapter {
  virtual auto size(void* instance) const -> size_t = 0;
  virtual auto item(void* instance, size_t idx) const -> void* = 0;
  virtual void resize(void* instance, size_t newSize) const = 0;
  virtual auto getSyntheticField() const
      -> std::shared_ptr<SyntheticArrayField> = 0;
  virtual auto type() const -> std::type_index = 0;
};

template <class TElem>
struct TFieldArrayAdapter : FieldArrayAdapter {
  auto size(void* instance) const -> size_t override {
    auto pA = reinterpret_cast<std::vector<TElem>*>(instance);
    return pA->size();
  }

  auto item(void* instance, size_t idx) const -> void* override {
    auto pA = reinterpret_cast<std::vector<TElem>*>(instance);
    return &pA[idx];
  }

  void resize(void* instance, size_t newSize) const override {
    auto pA = reinterpret_cast<std::vector<TElem>*>(instance);
    pA->resize(newSize);
  }

  auto type() const -> std::type_index override { return typeid(TElem); }

  auto getSyntheticField() const
      -> std::shared_ptr<SyntheticArrayField> override {
    return std::make_shared<TSyntheticArrayField<TElem>>();
  }
};

// Specialization for arrays
template <class C, class T>
class TypedField<C, std::vector<T>> : public Field {
 public:
  using FieldType = std::vector<T>;
  using FieldPtr = FieldType C::*;

  TypedField(std::string name, FieldPtr field)
      : Field(std::move(name), typeid(FieldType)),
        field_(field),
        adapter_(std::make_shared<TFieldArrayAdapter<T>>()) {}

  void setValue(
      void* instance, std::type_index srcType, void const* srcValue) override {
    assert(instance);
    assert(srcType == type());
    auto cp = static_cast<C*>(instance);
    auto fp = static_cast<FieldType const*>(srcValue);
    (cp->*field_) = *fp;
  }

  auto getValue(void* instance, std::type_index expected) const
      -> void* override {
    assert(instance);
    assert(expected == type());
    auto cp = static_cast<C*>(instance);
    return &(cp->*field_);
  }

  auto getArrayAdapter() const -> FieldArrayAdapterPtr override {
    return adapter_;
  }

 private:
  FieldPtr field_;
  FieldArrayAdapterPtr adapter_;
};

template <class C, class F>
auto Class::field(F C::*m, std::string name) -> Class& {
  fields_.push_back(std::make_shared<TypedField<C, F>>(name, m));
  nameToField_.emplace(std::move(name), fields_.back());
  return *this;
}
} // namespace ewok

static void ewok_auto_register_reflection_function_();

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
  friend void ::ewok_auto_register_reflection_function_();