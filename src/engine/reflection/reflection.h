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
#include <optional>
#include <span>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ewok {
class Class;
using ClassPtr = Class const*;

class Field;
using FieldPtr = Field const*;

class Enum;
using EnumPtr = Enum const*;

struct IArrayField;
using ArrayFieldPtr = IArrayField const*;

namespace detail {
template <class T>
class TClassBuilder;
} // namespace detail

class MemberInfo {
 public:
  virtual ~MemberInfo() = default;

  // Not copyable nor movable.
  MemberInfo(MemberInfo&&) noexcept = delete;
  MemberInfo(MemberInfo const&) = delete;
  MemberInfo& operator=(MemberInfo&&) noexcept = delete;
  MemberInfo& operator=(MemberInfo const&) = delete;

  auto name() const -> std::string const& { return name_; }
  auto type() const -> std::type_index { return type_; }

 protected:
  MemberInfo(std::string name, std::type_index type)
      : name_(std::move(name)), type_(type) {}

 private:
  std::string name_;
  std::type_index type_;
};

class Class : public MemberInfo {
 public:
  Class(
      std::string name,
      std::type_index type,
      std::vector<std::unique_ptr<Field>> fields);

  ~Class() override;

  auto fields() const -> std::span<FieldPtr const> { return fields_; }

  auto field(std::string const& name) const -> FieldPtr {
    if (auto it = nameToField_.find(name); it != nameToField_.end()) {
      return it->second;
    }

    return nullptr;
  }

 private:
  std::vector<FieldPtr> fields_;
  std::unordered_map<std::string, FieldPtr> nameToField_;
};

class Field : public MemberInfo {
 public:
  virtual auto asArray() const -> ArrayFieldPtr { return nullptr; }

  virtual void getValue(
      void* instance, std::type_index expected, void* out) const = 0;

  virtual void setValue(
      void* instance, std::type_index expected, void const* in) const = 0;

  virtual auto valuePtr(void* instance) const -> void* = 0;

  template <class T>
  T getValue(void* instance) const {
    T val;
    getValue(instance, typeid(T), &val);
    return val;
  }

  template <class T>
  void setValue(void* instance, T val) const {
    setValue(instance, typeid(T), &val);
  }

 protected:
  using MemberInfo::MemberInfo;
};

struct IArrayField {
  virtual auto getElemClass() const -> ClassPtr = 0;
  virtual auto getElemType() const -> std::type_index = 0;

  virtual auto getSize(void* instance) const -> size_t = 0;

  virtual void setSize(void* instance, size_t size) const = 0;

  virtual void swapElems(void* instance, size_t aIdx, size_t bIdx) const = 0;

  virtual void getValueAtIndex(
      void* instance, size_t index, std::type_index expected, void* out)
      const = 0;

  virtual void setValueAtIndex(
      void* instance, size_t index, std::type_index expected, void const* in)
      const = 0;

  virtual auto valueAtIndexPtr(void* instance, size_t index) const -> void* = 0;
  template <class T>
  T getValueAtIndex(void* instance, size_t index) const {
    T val;
    getValueAtIndex(instance, index, typeid(T), &val);
    return val;
  }

  template <class T>
  void setValueAtIndex(void* instance, size_t index, T const& val) {
    setValueAtIndex(instance, index, typeid(T), &val);
  }
};

class Enum : public MemberInfo {
 public:
  using MemberInfo::name;

  Enum(
      std::string name,
      std::type_index type,
      std::vector<std::pair<size_t, std::string>> valueNames);

  virtual auto name(void* ptr, std::type_index expected) const
      -> std::optional<std::string> = 0;

  virtual auto value(void* ptr, std::type_index expected) const
      -> std::optional<size_t> = 0;

  auto name(size_t val) const -> std::optional<std::string>;

  auto value(std::string const& name) const -> std::optional<size_t>;

  auto values() const -> std::span<std::pair<size_t, std::string> const> {
    return valueNames_;
  }

  virtual void setValue(void* fieldPtr, size_t value) const = 0;

 private:
  std::vector<std::pair<size_t, std::string>> valueNames_;
  std::unordered_map<size_t, std::string> valueToName_;
  std::unordered_map<std::string, size_t> nameToValue_;
};

namespace detail {
template <class C, class T>
class TMemberField : public Field {
 public:
  TMemberField(std::string name, T C::* ptr)
      : Field(std::move(name), typeid(T)), ptr_(ptr) {}

  void getValue(
      void* instance, std::type_index expected, void* out) const override {
    if (expected != this->type()) {
      throw std::runtime_error("Expected was not the correct type");
    }

    auto cp = reinterpret_cast<C const*>(instance);
    auto tp = reinterpret_cast<T*>(out);
    *tp = cp->*ptr_;
  }

  void setValue(
      void* instance, std::type_index expected, void const* in) const override {
    if (expected != this->type()) {
      throw std::runtime_error("Expected was not the correct type");
    }

    auto cp = reinterpret_cast<C*>(instance);
    auto tp = reinterpret_cast<T const*>(in);
    cp->*ptr_ = *tp;
  }

  auto valuePtr(void* instance) const -> void* override {
    auto cp = reinterpret_cast<C*>(instance);
    return &(cp->*ptr_);
  }

 protected:
  T C::* ptr_;
};

template <class T>
class TField;

template <class C, class T>
class TField<T C::*> : public TMemberField<C, T> {
 public:
  using TMemberField<C, T>::TMemberField;
};

template <class C, class T>
class TField<std::vector<T> C::*>
    : public TMemberField<C, std::vector<T>>, public IArrayField {
 public:
  using TMemberField<C, std::vector<T>>::TMemberField;

  auto asArray() const -> ArrayFieldPtr override { return this; }

  auto getElemClass() const -> ClassPtr override;

  auto getElemType() const -> std::type_index override { return typeid(T); }

  auto getSize(void* instance) const -> size_t override {
    auto vp = reinterpret_cast<std::vector<T> const*>(this->valuePtr(instance));
    return vp->size();
  }

  void setSize(void* instance, size_t size) const override {
    auto vp = reinterpret_cast<std::vector<T>*>(this->valuePtr(instance));
    vp->resize(size);
  }

  void swapElems(void* instance, size_t aIdx, size_t bIdx) const override {
    auto vp = reinterpret_cast<std::vector<T>*>(this->valuePtr(instance));
    std::swap(vp->at(aIdx), vp->at(bIdx));
  }

  void getValueAtIndex(
      void* instance, size_t index, std::type_index expected, void* out)
      const override {
    if (expected != typeid(T)) {
      throw std::runtime_error("Array expected was not the correct type");
    }
    auto vp = reinterpret_cast<std::vector<T> const*>(this->valuePtr(instance));
    auto tp = reinterpret_cast<T*>(out);
    *tp = vp->at(index);
  }

  void setValueAtIndex(
      void* instance, size_t index, std::type_index expected, void const* in)
      const override {
    if (expected != typeid(T)) {
      throw std::runtime_error("Array expected was not the correct type");
    }
    auto vp = reinterpret_cast<std::vector<T>*>(this->valuePtr(instance));
    auto tp = reinterpret_cast<T const*>(in);
    vp->at(index) = *tp;
  }

  auto valueAtIndexPtr(void* instance, size_t index) const -> void* override {
    auto vp = reinterpret_cast<std::vector<T>*>(this->valuePtr(instance));
    return &vp->at(index);
  }
};

template <class T>
class TEnum : public Enum {
 public:
  using Enum::Enum;
  using Enum::name;

  auto name(void* ptr, std::type_index expected) const
      -> std::optional<std::string> override {
    if (typeid(T) != expected) {
      return std::nullopt;
    }

    size_t val = static_cast<size_t>(*static_cast<T*>(ptr));
    return name(val);
  }

  auto value(void* ptr, std::type_index expected) const
      -> std::optional<size_t> override {
    if (typeid(T) != expected) {
      return std::nullopt;
    }

    size_t val = static_cast<size_t>(*static_cast<T*>(ptr));
    return val;
  }

  void setValue(void* fieldPtr, size_t value) const override {
    *static_cast<T*>(fieldPtr) = static_cast<T>(value);
  }
};

template <class T>
class TClassBuilder {
 public:
  TClassBuilder(std::string name) : name_(std::move(name)) {}

  ~TClassBuilder();

  template <class F>
  auto field(F f, std::string name) -> TClassBuilder& {
    fields_.push_back(std::make_unique<TField<F>>(std::move(name), f));
    return *this;
  }

 private:
  std::string name_;
  std::vector<std::unique_ptr<Field>> fields_;
};

template <class T>
class TEnumBuilder {
 public:
  TEnumBuilder(std::string name) : name_(std::move(name)) {}

  ~TEnumBuilder();

  auto value(T val, std::string name) -> TEnumBuilder& {
    valueNames_.push_back(
        std::make_pair(static_cast<size_t>(val), std::move(name)));
    return *this;
  }

 private:
  std::string name_;
  std::vector<std::pair<size_t, std::string>> valueNames_;
};
} // namespace detail

class Reflection {
 public:
  static auto class_(std::string const& name) -> ClassPtr;
  static auto class_(std::type_index type) -> ClassPtr;

  static auto enum_(std::string const& name) -> EnumPtr;
  static auto enum_(std::type_index type) -> EnumPtr;

 private:
  template <class T>
  friend class detail::TClassBuilder;

  template <class T>
  friend class detail::TEnumBuilder;

  friend struct Register;

  static std::unordered_map<std::string, ClassPtr>& nameToClass();
  static std::unordered_map<std::type_index, ClassPtr>& typeToClass();

  static std::unordered_map<std::string, EnumPtr>& nameToEnum();
  static std::unordered_map<std::type_index, EnumPtr>& typeToEnum();
};

struct Register {
  template <class T>
  static auto class_(std::string name) -> detail::TClassBuilder<T> {
    return detail::TClassBuilder<T>(std::move(name));
  }

  template <class T>
  static auto enum_(std::string name) -> detail::TEnumBuilder<T> {
    return detail::TEnumBuilder<T>(std::move(name));
  }
};

namespace detail {
template <class C, class T>
auto TField<std::vector<T> C::*>::getElemClass() const -> ClassPtr {
  return Reflection::class_(typeid(T));
}

template <class T>
TClassBuilder<T>::~TClassBuilder() {
  if (name_.empty()) {
    return;
  }

  auto cp = new Class(std::move(name_), typeid(T), std::move(fields_));

  Reflection::nameToClass().emplace(cp->name(), cp);
  Reflection::typeToClass().emplace(typeid(T), cp);
}

template <class T>
TEnumBuilder<T>::~TEnumBuilder() {
  if (name_.empty()) {
    return;
  }

  auto cp =
      new detail::TEnum<T>(std::move(name_), typeid(T), std::move(valueNames_));

  Reflection::nameToEnum().emplace(cp->name(), cp);
  Reflection::typeToEnum().emplace(typeid(T), cp);
}
} // namespace detail
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