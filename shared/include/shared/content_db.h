/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "shared/serialization.h"

#include <crossguid/guid.hpp>

namespace ewok::shared {
struct ContentDef {
  xg::Guid id;
};

struct IContentDb {
  virtual ~IContentDb() = default;

  virtual std::shared_ptr<void> get(xg::Guid const& id) = 0;

  template <class T>
    requires(std::is_base_of_v<ContentDef, T>)
  std::shared_ptr<T> get(xg::Guid const& id) {
    return std::static_pointer_cast<T>(get(id));
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

  ContentPtr(ContentPtr const&) = default;
  ContentPtr& operator=(ContentPtr const&) = default;

  ContentPtr(ContentPtr&&) = default;
  ContentPtr& operator=(ContentPtr&&) = default;

  std::shared_ptr<T> const& ptr() const {
    if (zero_ != 0) [[unlikely]] {
      ptr_ = getContentDb()->get<T>(id_);
      zero_ = 0;
    }

    return ptr;
  }

  auto guid() const -> xg::Guid const& {
    if (zero_ != 0) [[unlikely]] {
      return id_;
    }

    return ptr_->id;
  }

  T* operator->() const {
    return ptr().get();
  }

  T& operator*() const {
    return *ptr();
  }

private:
  union {
    // GUID of the content item. This is only valid if zero_ != 0. The high part of this guid is never zero.
    xg::Guid id_{};

    struct {
      // Flag that indicates this ID has been set. We do not allow the high part of our GUIDs to be non-zero
      mutable uint64_t zero_;
      // Pointer to the underlying item. This is only valid if zero_ == 0
      mutable std::shared_ptr<T> ptr_;
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

    value = {guid};
    return {};
  }
};
