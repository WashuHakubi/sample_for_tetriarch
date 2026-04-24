/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <wut/scheduler.h>

#include <cassert>

namespace wut {
void scheduler::schedule_op::await_suspend(std::coroutine_handle<> awaiting) noexcept {
  scheduler_.schedule(awaiting);
}

void manual_scheduler::schedule(std ::coroutine_handle<> h) {
  std::unique_lock lk{lock_};
  tasks_.emplace_back(h);
}

void manual_scheduler::execute(unsigned max_count) {
  unsigned executed = 0;

  while (true) {
    if (executed == max_count) {
      break;
    }

    // Lock to avoid conficts with scheduling from different threads.
    std::unique_lock lk{lock_};
    if (tasks_.empty())
      break;

    auto coro = tasks_.front();
    tasks_.pop_front();
    // Release the lock before executing the coroutine to avoid potential deadlocks.
    lk.unlock();

    coro.resume();
    ++executed;
  }
}

pool_scheduler::pool_scheduler(int thread_count) : run_{true} {
  if (thread_count < 0) {
    thread_count = static_cast<int>(std::thread::hardware_concurrency());
    assert(thread_count > 0);
  }

  for (int i = 0; i < thread_count; ++i) {
    workers_.emplace_back(std::thread([this]() { this->worker_entry(); }));
  }
}

void pool_scheduler::schedule(std ::coroutine_handle<> h) {
  if (!run_.load(std::memory_order::acquire)) {
    throw std::runtime_error("Pool scheduler is shutting down.");
  }

  std::unique_lock lk{lock_};
  tasks_.emplace_back(h);
  cv_.notify_one();
}

void pool_scheduler::stop() {
  {
    std::unique_lock lk{lock_};
    run_ = false;
    // Wake up all worker threads.
    cv_.notify_all();
  }

  // Wait for worker threads to finish.
  for (auto&& t : workers_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void pool_scheduler::worker_entry() {
  while (run_.load(std::memory_order::acquire)) {
    std::unique_lock lk{lock_};
    cv_.wait(lk, [this]() { return !tasks_.empty() || !run_.load(std::memory_order::acquire); });

    // Check if we have any tasks or if we're trying to stop the runner.
    if (tasks_.empty()) {
      continue;
    }

    auto coro = tasks_.front();
    tasks_.pop_front();

    // Release the lock before we execute the coroutine. This avoids potential deadlocks.
    lk.unlock();

    // Resume the coroutine.
    coro.resume();
  }

  // Flush any remaining coroutines.
  while (true) {
    std::unique_lock lk{lock_};
    if (tasks_.empty())
      break;

    auto coro = tasks_.front();
    tasks_.pop_front();
    lk.unlock();

    coro.resume();
  }
}
pool_scheduler::~pool_scheduler() {
  stop();
}
} // namespace wut

namespace wut::detail {
sync_wait_event::sync_wait_event(bool initial_value) : v_(initial_value) {}

void sync_wait_event::set() noexcept {
  std::unique_lock lk{lock_};
  v_.exchange(true, std::memory_order_seq_cst);
  cv_.notify_all();
}

void sync_wait_event::reset() noexcept {
  v_.exchange(false, std::memory_order_seq_cst);
}

void sync_wait_event::wait() noexcept {
  std::unique_lock lk{lock_};
  cv_.wait(lk, [this] { return v_.load(std::memory_order_seq_cst); });
}

auto sync_wait_promise<void>::get_return_object() noexcept {
  return coroutine_handle::from_promise(*this);
}
} // namespace wut::detail
