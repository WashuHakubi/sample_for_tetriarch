/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "shared/content_db.h"

namespace ewok::shared {
static IContentDbPtr s_contentDb;

void initializeContentDb(IContentDbPtr const& ptr) {
  assert(s_contentDb == nullptr);
  s_contentDb = ptr;
}

auto getContentDb() -> IContentDbPtr const& {
  assert(s_contentDb != nullptr);
  return s_contentDb;
}
}
