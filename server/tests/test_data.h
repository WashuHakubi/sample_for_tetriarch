/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <shared/design_data/testing/fake_content_db.h>

namespace ewok {
void populateDb(shared::design_data::FakeContentDb& contentDb);
}