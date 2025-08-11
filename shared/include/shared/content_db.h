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
struct ContentDef {
  xg::Guid id;
};

struct IContentDb {
  virtual ~IContentDb() = default;

  virtual void const* get(xg::Guid const& id, std::type_index type) = 0;

  template <class T>
    requires(std::is_base_of_v<ContentDef, T>)
  T const* get(xg::Guid const& id) {
    return static_cast<T const*>(get(id, typeid(T)));
  }
};

using IContentDbPtr = std::shared_ptr<IContentDb>;

// Initializes the content db.
void initializeContentDb(IContentDbPtr const& ptr);

// Gets the content db.
auto getContentDb() -> IContentDbPtr const&;

/// A pointer to a piece of content. This is serialized as the content item's GUID.
template <class T>
struct ContentPtr {
  ContentPtr() = default;

  explicit ContentPtr(xg::Guid const& id)
    : id_(id) {
  }

  ContentPtr(ContentPtr const& other) {
    if (other.zero_) {
      id_ = other.id_;
    } else {
      zero_ = 0;
      ptr_ = other.ptr_;
    }
  }

  ContentPtr(ContentPtr&& other) noexcept {
    if (other.zero_) {
      id_ = other.id_;
    } else {
      zero_ = 0;
      ptr_ = std::exchange(other.ptr_, nullptr);
    }
  }

  ContentPtr& operator=(ContentPtr const& other) {
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

  ContentPtr& operator=(ContentPtr&& other) noexcept {
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

  auto guid() const -> xg::Guid const& {
    if (zero_ != 0) [[unlikely]] {
      return id_;
    }

    return ptr_->id;
  }

  T const* operator->() const {
    return ptr();
  }

  T const& operator*() const {
    return *ptr();
  }

private:
  T const* ptr() const {
    if (zero_ != 0) [[unlikely]] {
      ptr_ = getContentDb()->get<T>(id_);
      zero_ = 0;
    }

    return ptr_;
  }

  union {
    // GUID of the content item. This is only valid if zero_ != 0. The high part of this guid is never zero.
    xg::Guid id_{};

    struct {
      // Flag that indicates this ID has been set. We do not allow the high part of our GUIDs to be non-zero
      mutable uint64_t zero_;
      // Pointer to the underlying item. This is only valid if zero_ == 0
      mutable T const* ptr_;
    };
  };
};
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
