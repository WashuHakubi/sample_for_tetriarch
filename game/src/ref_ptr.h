/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <concepts>
#include <functional>
#include <utility>

namespace wut {
template <class T>
concept reference_counted = requires(T t) {
  { t.add_ref() } -> std::same_as<void>;
  { t.release() } -> std::same_as<void>;
};

template <class T>
struct ref_count_mixin {
  ref_count_mixin() = default;

  void add_ref() { ref_count_.fetch_add(1, std::memory_order_relaxed); }

  void release() {
    if (ref_count_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete static_cast<T*>(this);
    }
  }

 private:
  std::atomic_uint32_t ref_count_{0};
};

template <class T>
struct ref_ptr {
  ref_ptr() noexcept = default;

  ref_ptr(T* ptr) noexcept : ptr_(ptr) {
    if (ptr_) {
      ptr_->add_ref();
    }
  }

  ref_ptr(std::nullptr_t) noexcept {}

  ref_ptr(ref_ptr const& o) : ptr_(o.ptr_) {
    if (ptr_) {
      ptr_->add_ref();
    }
  }

  ref_ptr(ref_ptr&& o) noexcept : ptr_(std::exchange(o.ptr_, nullptr)) {}

  ~ref_ptr() {
    static_assert(reference_counted<T>);
    if (ptr_) {
      ptr_->release();
    }
  }

  ref_ptr& operator=(ref_ptr const& o) noexcept {
    if (std::addressof(o) != this) {
      if (ptr_) {
        ptr_->release();
      }
      ptr_ = o.ptr_;
      ptr_->add_ref();
    }
    return *this;
  }

  ref_ptr& operator=(ref_ptr&& o) noexcept {
    if (std::addressof(o) != this) {
      if (ptr_) {
        ptr_->release();
      }
      ptr_ = std::exchange(o.ptr_, nullptr);
    }
    return *this;
  }

  auto get() noexcept -> T* { return ptr_; }

  auto get() const noexcept -> T const* { return ptr_; }

  /**
   * Releases ownership of the contained pointer. The reference count is not decremented.
   */
  auto release() noexcept { return std::exchange(ptr_, nullptr); }

  auto operator*() noexcept -> T& {
    assert(ptr_);
    return *ptr_;
  }

  auto operator*() const noexcept -> T const& {
    assert(ptr_);
    return *ptr_;
  }

  auto operator->() noexcept -> T* { return ptr_; }

  auto operator->() const noexcept -> T const* { return ptr_; }

  bool operator==(ref_ptr const& o) const noexcept { return o.ptr_ == ptr_; }

  bool operator!=(ref_ptr const& o) const noexcept { return o.ptr_ != ptr_; }

 private:
  T* ptr_{nullptr};
};

template <reference_counted T, class... TArgs>
ref_ptr<T> make_ref_counted(TArgs&&... args) {
  auto p = new T(std::forward<TArgs>(args)...);
  return {p};
}
} // namespace wut

namespace std {
template <wut::reference_counted T>
struct hash<wut::ref_ptr<T>> {
  size_t operator()(wut::ref_ptr<T> const& ptr) const noexcept { return std::hash<T*>{}(ptr.get()); }
};
} // namespace std
