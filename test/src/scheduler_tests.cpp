/*
 * ------
 * Copyright (c) 2026 Sean Kent
 * All rights reserved.
 */

#include <catch2/catch_all.hpp>

#include <wut/scheduler.h>
#include <wut/task.h>

TEST_CASE("pool_scheduler", "[task]") {
  auto s = wut::pool_scheduler();

  auto func = [](wut::pool_scheduler& s) -> wut::lazy_task<uint64_t> {
    co_await s.schedule(); // Schedule this coroutine on the scheduler.
    co_return 42;
  };

  auto result = wut::sync_wait(func(s));
  REQUIRE(result == 42);
}
