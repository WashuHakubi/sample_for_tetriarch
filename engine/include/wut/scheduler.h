/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#pragma once

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <coroutine>
#include <deque>
#include <mutex>
#include <thread>

namespace wut {
template <class T>
concept is_suspend_result_type =
    (std::same_as<T, void> || std::same_as<T, bool> || std::same_as<T, std::coroutine_handle<>>);

template <class T>
concept await_type = requires(T t, std::coroutine_handle<> h) {
  { t.await_ready() } -> std::same_as<bool>;
  { t.await_resume() };
  { t.await_suspend(h) } -> is_suspend_result_type;
};

template <class T>
concept has_co_await_operator = requires(T t) {
  { t.operator co_await() } -> await_type;
};

template <class T>
concept has_co_await_global_operator = requires(T t) {
  { operator co_await(t) } -> await_type;
};

template <class T>
concept awaitable = await_type<T> || has_co_await_global_operator<T> || has_co_await_operator<T>;

template <awaitable T>
static auto get_awaiter(T&& t) {
  if constexpr (has_co_await_global_operator<T>) {
    return operator co_await(std::forward<T>(t));
  } else if constexpr (has_co_await_operator<T>) {
    return std::forward<T>(t).operator co_await();
  } else {
    return std::forward<T>(t);
  }
}

template <awaitable T>
using awaitable_type = decltype(get_awaiter(std::declval<T>()));

template <awaitable T>
using awaitable_result_type = decltype(std::declval<awaitable_type<T>>().await_resume());

struct scheduler {
  struct schedule_op {
    schedule_op(scheduler& scheduler) : scheduler_(scheduler) {}

    bool await_ready() const noexcept { return false; }

    void await_resume() const noexcept {}

    void await_suspend(std::coroutine_handle<> awaiting) noexcept;

   private:
    scheduler& scheduler_;
  };

  virtual ~scheduler() = default;

  /**
   * Schedules a coroutine to run on this scheduler.
   */
  virtual void schedule(std::coroutine_handle<> h) = 0;

  /**
   * Schedules this coroutine to run on this scheduler.
   */
  schedule_op schedule() { return schedule_op{*this}; }
};

class manual_scheduler : public scheduler {
 public:
  using scheduler::schedule;

  /**
   * Schedules a coroutine to run on this scheduler.
   */
  void schedule(std ::coroutine_handle<> h) override;

  /**
   * Resumes up to max_count coroutines. Returns when either the queue is empty or max_count coroutines have executed.
   */
  void execute(unsigned max_count);

 private:
  std::mutex lock_;
  std::deque<std::coroutine_handle<>> tasks_;
};

/**
 * Thread pool based scheduler.
 */
class pool_scheduler : public scheduler {
 public:
  using scheduler::schedule;

  /**
   * Constructs a thread pool scheduler. If thread_count is negative then the number of threads created will match the
   * hardware concurrency.
   */
  pool_scheduler(int thread_count = -1);

  ~pool_scheduler();

  /**
   * Schedules a coroutine to run on this scheduler.
   */
  void schedule(std ::coroutine_handle<> h) override;

  /**
   * Stops the pool scheduler. Will wait till all threads in the pool have stopped.
   */
  void stop();

 private:
  void worker_entry();

  std::mutex lock_;
  std::atomic_bool run_{true};
  std::deque<std::coroutine_handle<>> tasks_;
  std::vector<std::thread> workers_;
  std::condition_variable cv_;
};
} // namespace wut

namespace wut::detail {
template <class T>
struct sync_wait_task;

struct sync_wait_event {
  sync_wait_event(bool initial_value = false);

  sync_wait_event(sync_wait_event const&) = delete;
  sync_wait_event(sync_wait_event&&) noexcept = delete;
  sync_wait_event& operator=(sync_wait_event const&) = delete;
  sync_wait_event& operator=(sync_wait_event&&) noexcept = delete;

  void set() noexcept;
  void reset() noexcept;
  void wait() noexcept;

 private:
  std::mutex lock_;
  std::condition_variable cv_;
  std::atomic_bool v_;
};

template <class T>
struct sync_wait_promise {
  using coroutine_handle = std::coroutine_handle<sync_wait_promise<T>>;
  using task_type = sync_wait_task<T>;
  using storage_type = std::conditional_t<std::is_reference_v<T>, std::remove_cvref_t<T>*, std::remove_cv_t<T>>;

  auto initial_suspend() const noexcept -> std::suspend_always { return {}; }

  auto get_return_object() noexcept { return coroutine_handle::from_promise(*this); }

  template <class U>
    requires(std::is_constructible_v<T, U &&>)
  void return_value(U&& v) noexcept {
    if constexpr (std::is_reference_v<T>) {
      T ref = static_cast<U&&>(v);
      value_.template emplace<storage_type>(std::addressof(ref));
    } else {
      value_.template emplace<storage_type>(std::forward<U>(v));
    }
  }

  void unhandled_exception() noexcept { value_.template emplace<std::exception_ptr>(std::current_exception()); }

  auto final_suspend() noexcept {
    struct notifier {
      bool await_ready() const noexcept { return false; }
      void await_resume() const noexcept {}
      void await_suspend(coroutine_handle h) const noexcept { h.promise().event_->set(); }
    };
    return notifier{};
  }

  auto result() & -> decltype(auto) {
    if (auto except = std::get_if<std::exception_ptr>(&value_)) {
      std::rethrow_exception(*except);
    } else if (std::holds_alternative<storage_type>(value_)) {
      if constexpr (std::is_reference_v<T>) {
        return static_cast<T>(*std::get<storage_type>(value_));
      } else {
        return std::get<storage_type>(value_);
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
        return std::get<storage_type>(value_);
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
        return std::get<storage_type>(value_);
      }
    }
    throw std::runtime_error("Result not set");
  }

  void start(sync_wait_event& evt) {
    event_ = &evt;
    coroutine_handle::from_promise(*this).resume();
  }

 private:
  sync_wait_event* event_{nullptr};
  std::variant<std::monostate, storage_type, std::exception_ptr> value_;
};

template <>
struct sync_wait_promise<void> {
  using coroutine_handle = std::coroutine_handle<sync_wait_promise<void>>;
  using task_type = sync_wait_task<void>;

  auto initial_suspend() const noexcept -> std::suspend_always { return {}; }

  auto get_return_object() noexcept;

  void return_void() noexcept {}

  void unhandled_exception() noexcept { except_ = std::current_exception(); }

  auto final_suspend() noexcept {
    struct notifier {
      bool await_ready() const noexcept { return false; }
      void await_resume() const noexcept {}
      void await_suspend(coroutine_handle h) const noexcept { h.promise().event_->set(); }
    };
    return notifier{};
  }

  auto result() {
    if (except_) {
      std::rethrow_exception(except_);
    }
  }

  void start(sync_wait_event& evt) {
    event_ = &evt;
    coroutine_handle::from_promise(*this).resume();
  }

 private:
  sync_wait_event* event_{nullptr};
  std::exception_ptr except_;
};

template <class T>
struct sync_wait_task {
  using promise_type = sync_wait_promise<T>;
  using coroutine_handle = std::coroutine_handle<promise_type>;

  sync_wait_task(coroutine_handle h) : h_(h) {}

  sync_wait_task(sync_wait_task&& o) noexcept : h_(std::exchange(o.h_, nullptr)) {}

  sync_wait_task(sync_wait_task const&) = delete;

  sync_wait_task& operator=(sync_wait_task const&) = delete;

  ~sync_wait_task() {
    if (h_) {
      h_.destroy();
    }
  }

  sync_wait_task& operator=(sync_wait_task&& o) noexcept {
    if (std::addressof(o) != this) {
      if (h_) {
        h_.destroy();
      }
      h_ = std::exchange(o.h_, nullptr);
    }
    return *this;
  };

  auto promise() & -> promise_type& { return h_.promise(); }

  auto promise() const& -> const promise_type& { return h_.promise(); }

  auto promise() && -> promise_type&& { return std::move(h_.promise()); }

 private:
  coroutine_handle h_;
};

template <awaitable T, class TResult = awaitable_result_type<T>>
auto create_sync_wait_task(T&& t) -> sync_wait_task<TResult> {
  if constexpr (std::is_void_v<TResult>) {
    co_await std::forward<T>(t);
    co_return;
  } else {
    co_return co_await std::forward<T>(t);
  }
}
} // namespace wut::detail

namespace wut {
template <awaitable T>
auto sync_wait(T&& t) {
  detail::sync_wait_event e;
  auto task = detail::create_sync_wait_task(std::forward<T>(t));
  task.promise().start(e);
  e.wait();

  return std::move(task).promise().result();
}
} // namespace wut
