/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <catch2/catch_all.hpp>
#include <shared/sparse_set.h>

using namespace ewok::shared;

TEST_CASE("Can construct sparse set") {
  SparseSet s;

  s.insert(Entity{});

  REQUIRE(s.contains(Entity{}));

  s.insert(Entity{1});
  REQUIRE(s.contains(Entity{1}));

  s.insert(Entity{2});
  REQUIRE(s.contains(Entity{2}));
  REQUIRE(s.size() == 3);

  {
    // Since we traverse the container backwards, we expect the entities to be returned in the opposite order they were
    // inserted.
    auto expected = {
        Entity{2},
        Entity{1},
        Entity{},
    };

    auto j = expected.begin();
    for (auto i = s.begin(); i != s.end(); ++i) {
      REQUIRE(s.contains(*j));
      REQUIRE(*i == *j++);
    }
    REQUIRE(j == expected.end());
  }

  {
    auto i = s.erase(Entity{});
    REQUIRE(!s.contains(Entity{}));
    REQUIRE(s.contains(Entity{1}));
    REQUIRE(s.contains(Entity{2}));
    REQUIRE(s.size() == 2);
    REQUIRE(i == s.end());
  }

  {
    // We expect entity 2 and entity 0 to have swapped places, and entity 0 to have been removed. As Erase is a swap and pop operation.
    auto expected = {
        Entity{1},
        Entity{2},
    };

    auto j = expected.begin();
    for (auto i : s) {
      REQUIRE(i == *j++);
    }
    REQUIRE(j == expected.end());
  }

  {
    auto i = s.erase(Entity{1});
    REQUIRE(!s.contains(Entity{1}));
    REQUIRE(s.contains(Entity{2}));
    REQUIRE(i == s.begin());
    REQUIRE(*s.begin() == Entity{2});
    REQUIRE(s.size() == 1);
  }
}