/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <utility>
#include <variant>

namespace wut {
template <class T = void>
class task;

template <class T = void>
class lazy_task;
} // namespace wut

namespace wut::detail {
struct promise_base {
  struct final_awaiter {
    // Always suspend.
    bool await_ready() const noexcept { return false; }

    template <class TPromise>
    auto await_suspend(std::coroutine_handle<TPromise> h) const noexcept -> std::coroutine_handle<> {
      // If we were awaited upon, return it to resume the awaiter.
      auto precursor = h.promise().precursor_;
      if (precursor) {
        return precursor;
      }
      return std::noop_coroutine();
    }

    // do nothing.
    void await_resume() const noexcept {}
  };

  // Resume the awaiter, if any, when we are done running.
  auto final_suspend() const noexcept { return final_awaiter{}; }

  // Called to store the coroutine awaiting this promise.
  void precursor(std::coroutine_handle<> p) noexcept { precursor_ = p; }

 protected:
  std::coroutine_handle<> precursor_;
};

template <class T>
struct task_result_promise : promise_base {
  using storage_type = std::conditional_t<std::is_reference_v<T>, std::remove_cvref_t<T>*, std::remove_cv_t<T>>;

  // The coroutine threw an exception, capture it for the awaiter.
  void unhandled_exception() noexcept { value_ = std::current_exception(); }

  // Stores the return value from a co_return.
  void return_value(T value) noexcept { value_ = std::move(value); }

  auto result() & -> decltype(auto) {
    if (auto except = std::get_if<std::exception_ptr>(&value_)) {
      std::rethrow_exception(*except);
    } else if (std::holds_alternative<storage_type>(value_)) {
      if constexpr (std::is_reference_v<T>) {
        return static_cast<T>(*std::get<storage_type>(value_));
      } else {
        return static_cast<T&>(std::get<storage_type>(value_));
      }
    }
    throw std::runtime_error("Result not set");
  }

  auto result() const& -> decltype(auto) {
    if (auto except = std::get_if<std::exception_ptr>(&value_)) {
      std::rethrow_exception(*except);
    } else if (std::holds_alternative<storage_type>(value_)) {
      if constexpr (std::is_reference_v<T>) {
        return static_cast<std::add_const_t<T>>(*std::get<storage_type>(value_));
      } else {
        return static_cast<T const&>(std::get<storage_type>(value_));
      }
    }
    throw std::runtime_error("Result not set");
  }

  auto result() && -> decltype(auto) {
    if (auto except = std::get_if<std::exception_ptr>(&value_)) {
      std::rethrow_exception(*except);
    } else if (std::holds_alternative<storage_type>(value_)) {
      if constexpr (std::is_reference_v<T>) {
        return static_cast<T>(*std::get<storage_type>(value_));
      } else {
        return static_cast<T&&>(std::get<storage_type>(value_));
      }
    }
    throw std::runtime_error("Result not set");
  }

 private:
  std::variant<std::monostate, storage_type, std::exception_ptr> value_;
};

template <>
struct task_result_promise<void> : promise_base {
  // The coroutine threw an exception, capture it for the awaiter.
  void unhandled_exception() noexcept { except_ = std::current_exception(); }

  // Stores the return value from a co_return.
  void return_void() const noexcept {}

  void result() {
    if (except_) {
      std::rethrow_exception(except_);
    }
  }

 private:
  std::exception_ptr except_;
};

template <class T>
struct lazy_task_promise : task_result_promise<T> {
  using task_type = lazy_task<T>;
  using coroutine_handle = typename task_type::coroutine_handle;

  // Gets the task to return when this promise is created.
  task_type get_return_object() noexcept { return {coroutine_handle::from_promise(*this)}; }

  // The coroutine always starts suspended
  auto initial_suspend() const noexcept -> std::suspend_always { return {}; }
};

template <class T>
struct task_promise : task_result_promise<T> {
  using task_type = task<T>;
  using coroutine_handle = typename task_type::coroutine_handle;

  // Gets the task to return when this promise is created.
  task_type get_return_object() noexcept { return {coroutine_handle::from_promise(*this)}; }

  // The coroutine always starts suspended
  auto initial_suspend() const noexcept -> std::suspend_never { return {}; }
};
} // namespace wut::detail

namespace wut {
template <class T>
class lazy_task {
 public:
  using promise_type = detail::lazy_task_promise<T>;
  using coroutine_handle = std::coroutine_handle<promise_type>;

  struct task_awaitable {
    task_awaitable(coroutine_handle h) noexcept : h_(h) {}

    bool await_ready() const noexcept { return !h_ || h_.done(); }

    auto await_suspend(std::coroutine_handle<> coro) noexcept -> std::coroutine_handle<> {
      h_.promise().precursor(coro);
      return h_;
    }

   protected:
    coroutine_handle h_;
  };

  lazy_task() noexcept = default;

  lazy_task(coroutine_handle h) noexcept : h_(h) {}

  // Not copy-constructible
  lazy_task(lazy_task const&) = delete;

  lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, nullptr)) {}

  ~lazy_task() {
    if (h_) {
      h_.destroy();
    }
  }

  // Not copy-assignable
  lazy_task& operator=(lazy_task const&) = delete;

  lazy_task& operator=(lazy_task&& o) noexcept {
    if (std::addressof(o) != this) {
      if (h_) {
        h_.destroy();
      }
      h_ = std::exchange(o.h_, nullptr);
    }
    return *this;
  }

  bool done() const noexcept { return h_ == nullptr || h_.done(); }

  // Attempts to resume a coroutine, returns false if the coroutine is done.
  bool resume() {
    if (!h_.done()) {
      h_.resume();
    }
    return !h_.done();
  }

  auto handle() const noexcept -> coroutine_handle { return h_; }

  auto promise() & noexcept -> promise_type& { return h_.promise(); }

  auto promise() const& noexcept -> promise_type const& { return h_.promise(); }

  auto promise() && -> promise_type&& { return std::move(h_.promise()); }

  auto operator co_await() const& noexcept {
    struct awaitable : task_awaitable {
      auto await_resume() -> decltype(auto) { return this->h_.promise().result(); }
    };
    return awaitable{h_};
  }

  auto operator co_await() const&& noexcept {
    struct awaitable : task_awaitable {
      auto await_resume() -> decltype(auto) { return std::move(this->h_.promise()).result(); }
    };
    return awaitable{h_};
  }

 private:
  coroutine_handle h_;
};

template <class T>
class task {
 public:
  using promise_type = detail::task_promise<T>;
  using coroutine_handle = std::coroutine_handle<promise_type>;

  struct task_awaitable {
    task_awaitable(coroutine_handle h) noexcept : h_(h) {}

    bool await_ready() const noexcept { return !h_ || h_.done(); }

    auto await_suspend(std::coroutine_handle<> coro) noexcept -> std::coroutine_handle<> {
      h_.promise().precursor(coro);
      return h_;
    }

   protected:
    coroutine_handle h_;
  };

  task() noexcept = default;

  task(coroutine_handle h) noexcept : h_(h) {}

  // Not copy-constructible
  task(task const&) = delete;

  task(task&& o) noexcept : h_(std::exchange(o.h_, nullptr)) {}

  ~task() {
    if (h_) {
      h_.destroy();
    }
  }

  // Not copy-assignable
  task& operator=(task const&) = delete;

  task& operator=(task&& o) noexcept {
    if (std::addressof(o) != this) {
      if (h_) {
        h_.destroy();
      }
      h_ = std::exchange(o.h_, nullptr);
    }
    return *this;
  }

  bool done() const noexcept { return h_ == nullptr || h_.done(); }

  // Attempts to resume a coroutine, returns false if the coroutine is done.
  bool resume() {
    if (!h_.done()) {
      h_.resume();
    }
    return !h_.done();
  }

  auto handle() const noexcept -> coroutine_handle { return h_; }

  auto promise() & noexcept -> promise_type& { return h_.promise(); }

  auto promise() const& noexcept -> promise_type const& { return h_.promise(); }

  auto promise() && -> promise_type&& { return std::move(h_.promise()); }

  auto operator co_await() const& noexcept {
    struct awaitable : task_awaitable {
      auto await_resume() -> decltype(auto) { return this->h_.promise().result(); }
    };
    return awaitable{h_};
  }

  auto operator co_await() const&& noexcept {
    struct awaitable : task_awaitable {
      auto await_resume() -> decltype(auto) { return std::move(this->h_.promise()).result(); }
    };
    return awaitable{h_};
  }

 private:
  coroutine_handle h_;
};
} // namespace wut
