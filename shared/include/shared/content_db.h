/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <typeindex>

#include "shared/serialization.h"

#include <crossguid/guid.hpp>

namespace ewok::shared {
struct IContentDb;

struct ContentDef {
  xg::Guid id;
};

enum class ContentScope {
  // Content that is shared across the entire game
  Global,

  // Content specific to a map
  Map,
};

/// A pointer to a piece of content. This is serialized as the content item's GUID.
template <class T>
struct ContentPtr {
  ContentPtr() = default;

  explicit constexpr ContentPtr(xg::Guid const& id);

  explicit constexpr ContentPtr(T const* p);

  constexpr ContentPtr(ContentPtr const& other);

  constexpr ContentPtr(ContentPtr&& other) noexcept;

  constexpr ContentPtr& operator=(ContentPtr const& other);

  constexpr ContentPtr& operator=(ContentPtr&& other) noexcept;

  auto guid() const -> xg::Guid const&;

  T const* resolve(IContentDb& contentDb) const;

private:
  union {
    // GUID of the content item. This is only valid if zero_ != 0. The high part of this guid is never zero when valid.
    xg::Guid id_{};

    struct {
      // Flag that indicates this ID has been set. We do not allow the high part of our GUIDs to be non-zero
      mutable uint64_t zero_;
      // Pointer to the underlying item. This is only valid if zero_ == 0
      mutable T const* ptr_;
    };
  };
};

struct IContentDb {
public:
  virtual ~IContentDb() = default;

  template <class T>
    requires(std::is_base_of_v<ContentDef, T>)
  T const* get(xg::Guid const& id) {
    return static_cast<T const*>(get(id, typeid(T)));
  }

  template <class T>
    requires(std::is_base_of_v<ContentDef, T>)
  std::vector<ContentPtr<T>> getAllInScope(ContentScope scope) {
    std::vector<ContentPtr<T>> result;
    auto const ptrs = getAllInScope(typeid(T), scope);
    result.reserve(ptrs.size());
    for (auto const& ptr : ptrs) {
      result.emplace_back(ContentPtr<T>(static_cast<T const*>(ptr)));
    }
    return result;
  }

protected:
  virtual void const* get(xg::Guid const& id, std::type_index type) = 0;

  virtual std::vector<void const*> getAllInScope(std::type_index type, ContentScope scope) = 0;
};

using IContentDbPtr = std::shared_ptr<IContentDb>;

template <class T>
constexpr ContentPtr<T>::ContentPtr(xg::Guid const& id)
  : id_(id) {
  static_assert(std::is_base_of_v<ContentDef, T>);
}

template <class T>
constexpr ContentPtr<T>::ContentPtr(T const* p)
  : zero_(0),
    ptr_(p) {
  static_assert(std::is_base_of_v<ContentDef, T>);
}

template <class T>
constexpr ContentPtr<T>::ContentPtr(ContentPtr const& other) {
  if (other.zero_) {
    id_ = other.id_;
  } else {
    zero_ = 0;
    ptr_ = other.ptr_;
  }
}

template <class T>
constexpr ContentPtr<T>::ContentPtr(ContentPtr&& other) noexcept {
  if (other.zero_) {
    id_ = other.id_;
  } else {
    zero_ = 0;
    ptr_ = std::exchange(other.ptr_, nullptr);
  }
}

template <class T>
constexpr ContentPtr<T>& ContentPtr<T>::operator=(ContentPtr const& other) {
  if (std::addressof(other) == this) {
    return *this;
  }

  if (other.zero_) {
    id_ = other.id_;
  } else {
    zero_ = 0;
    ptr_ = other.ptr_;
  }
  return *this;
}

template <class T>
constexpr ContentPtr<T>& ContentPtr<T>::operator=(ContentPtr&& other) noexcept {
  if (std::addressof(other) == this) {
    return *this;
  }

  if (other.zero_) {
    id_ = other.id_;
  } else {
    zero_ = 0;
    ptr_ = std::exchange(other.ptr_, nullptr);
  }
  return *this;
}

template <class T>
auto ContentPtr<T>::guid() const -> xg::Guid const& {
  if (zero_ != 0) [[unlikely]] {
    return id_;
  }

  return ptr_->id;
}

template <class T>
T const* ContentPtr<T>::resolve(IContentDb& contentDb) const {
  if (zero_ != 0) [[unlikely]] {
    ptr_ = contentDb.get<T>(id_);
    zero_ = 0;
  }

  return ptr_;
}
}

template <>
struct ewok::shared::serialization::CustomSerializable<xg::Guid> : std::true_type {
  using value_type = xg::Guid;

  static auto serialize(TSerializeWriter auto& writer, value_type const& value) -> Result {
    return writer.write("g", value.str());
  }

  static auto deserialize(TSerializeReader auto& reader, value_type& value) -> Result {
    std::string guid;
    if (auto r = reader.read("g", guid); !r) {
      return r;
    }
    value = xg::Guid(guid);
    return {};
  }
};

template <class T>
struct ewok::shared::serialization::CustomSerializable<ewok::shared::ContentPtr<T>> : std::true_type {
  using value_type = ewok::shared::ContentPtr<T>;

  static auto serialize(TSerializeWriter auto& writer, value_type const& value) -> Result {
    return CustomSerializable<xg::Guid>::serialize(writer, value.guid());
  }

  static auto deserialize(TSerializeReader auto& reader, value_type& value) -> Result {
    xg::Guid guid;
    if (auto r = CustomSerializable<xg::Guid>::deserialize(reader, guid); !r) {
      return r;
    }

    value = value_type{guid};
    return {};
  }
};
