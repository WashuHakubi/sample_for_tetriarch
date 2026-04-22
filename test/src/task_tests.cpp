/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <catch2/catch_all.hpp>

#include <wut/task.h>

#include <iostream>
#include <thread>

TEST_CASE("task hello world", "[task]") {
  using task_type = wut::task<std::string>;

  auto h = []() -> task_type { co_return "Hello"; }();
  auto w = []() -> task_type { co_return "World"; }();

  REQUIRE_THROWS_AS(h.promise().result(), std::runtime_error);
  REQUIRE_THROWS_AS(w.promise().result(), std::runtime_error);

  h.resume(); // task suspends immediately
  w.resume();

  REQUIRE(h.done());
  REQUIRE(w.done());

  auto w_value = std::move(w).promise().result();

  REQUIRE(h.promise().result() == "Hello");
  REQUIRE(w_value == "World");
  REQUIRE(w.promise().result().empty());
}

TEST_CASE("task void", "[task]") {
  using namespace std::chrono_literals;
  using task_type = wut::task<>;

  auto t = []() -> task_type {
    std::this_thread::sleep_for(10ms);
    co_return;
  }();
  t.resume();

  REQUIRE(t.done());
}

TEST_CASE("task exception thrown", "[task]") {
  using task_type = wut::task<std::string>;

  std::string throw_msg = "I'll be reached";

  auto task = [](std::string& throw_msg) -> task_type {
    throw std::runtime_error(throw_msg);
    co_return "I'll never be reached";
  }(throw_msg);

  task.resume();

  REQUIRE(task.done());

  bool thrown{false};
  try {
    auto value = task.promise().result();
  } catch (const std::exception& e) {
    thrown = true;
    REQUIRE(e.what() == throw_msg);
  }

  REQUIRE(thrown);
}

TEST_CASE("task in a task", "[task]") {
  auto outer_task = []() -> wut::task<> {
    auto inner_task = []() -> wut::task<int> {
      std::cerr << "inner_task start\n";
      std::cerr << "inner_task stop\n";
      co_return 42;
    };

    std::cerr << "outer_task start\n";
    auto v = co_await inner_task();
    REQUIRE(v == 42);
    std::cerr << "outer_task stop\n";
  }();

  outer_task.resume(); // all tasks start suspend, kick it off.

  REQUIRE(outer_task.done());
}

TEST_CASE("task in a task in a task", "[task]") {
  auto task1 = []() -> wut::task<> {
    std::cerr << "task1 start\n";
    auto task2 = []() -> wut::task<int> {
      std::cerr << "\ttask2 start\n";
      auto task3 = []() -> wut::task<int> {
        std::cerr << "\t\ttask3 start\n";
        std::cerr << "\t\ttask3 stop\n";
        co_return 3;
      };

      auto v2 = co_await task3();
      REQUIRE(v2 == 3);

      std::cerr << "\ttask2 stop\n";
      co_return 2;
    };

    auto v1 = co_await task2();
    REQUIRE(v1 == 2);

    std::cerr << "task1 stop\n";
  }();

  task1.resume(); // all tasks start suspended, kick it off.

  REQUIRE(task1.done());
}

TEST_CASE("task multiple suspends return void", "[task]") {
  auto task = []() -> wut::task<void> {
    co_await std::suspend_always{};
    co_await std::suspend_never{};
    co_await std::suspend_always{};
    co_await std::suspend_always{};
    co_return;
  }();

  task.resume(); // initial suspend
  REQUIRE_FALSE(task.done());

  task.resume(); // first internal suspend
  REQUIRE_FALSE(task.done());

  task.resume(); // second internal suspend
  REQUIRE_FALSE(task.done());

  task.resume(); // third internal suspend
  REQUIRE(task.done());
}

TEST_CASE("task multiple suspends return integer", "[task]") {
  auto task = []() -> wut::task<int> {
    co_await std::suspend_always{};
    co_await std::suspend_always{};
    co_await std::suspend_always{};
    co_return 11;
  }();

  task.resume(); // initial suspend
  REQUIRE_FALSE(task.done());

  task.resume(); // first internal suspend
  REQUIRE_FALSE(task.done());

  task.resume(); // second internal suspend
  REQUIRE_FALSE(task.done());

  task.resume(); // third internal suspend
  REQUIRE(task.done());
  REQUIRE(task.promise().result() == 11);
}

TEST_CASE("task resume from promise to coroutine handles of different types", "[task]") {
  auto task1 = []() -> wut::task<int> {
    std::cerr << "Task ran\n";
    co_return 42;
  }();

  auto task2 = []() -> wut::task<void> {
    std::cerr << "Task 2 ran\n";
    co_return;
  }();

  // task.resume();  normal method of resuming

  std::vector<std::coroutine_handle<>> handles;

  handles.emplace_back(std::coroutine_handle<wut::task<int>::promise_type>::from_promise(task1.promise()));
  handles.emplace_back(std::coroutine_handle<wut::task<void>::promise_type>::from_promise(task2.promise()));

  auto& coro_handle1 = handles[0];
  coro_handle1.resume();
  auto& coro_handle2 = handles[1];
  coro_handle2.resume();

  REQUIRE(task1.done());
  REQUIRE(coro_handle1.done());
  REQUIRE(task1.promise().result() == 42);

  REQUIRE(task2.done());
  REQUIRE(coro_handle2.done());
}

TEST_CASE("task throws void", "[task]") {
  auto task = []() -> wut::task<void> {
    throw std::runtime_error{"I always throw."};
    co_return;
  }();

  REQUIRE_NOTHROW(task.resume());
  REQUIRE(task.done());
  REQUIRE_THROWS_AS(task.promise().result(), std::runtime_error);
}

TEST_CASE("task throws non-void l-value", "[task]") {
  auto task = []() -> wut::task<int> {
    throw std::runtime_error{"I always throw."};
    co_return 42;
  }();

  REQUIRE_NOTHROW(task.resume());
  REQUIRE(task.done());
  REQUIRE_THROWS_AS(task.promise().result(), std::runtime_error);
}

TEST_CASE("task throws non-void r-value", "[task]") {
  struct type {
    int m_value;
  };

  auto task = []() -> wut::task<type> {
    type return_value{42};

    throw std::runtime_error{"I always throw."};
    co_return std::move(return_value);
  }();

  task.resume();
  REQUIRE(task.done());
  REQUIRE_THROWS_AS(task.promise().result(), std::runtime_error);
}
