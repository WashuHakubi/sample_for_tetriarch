/*
 *  Copyright (c) 2026 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <shared/reflection.h>

/// When present on an entity with a transform will display the coordinate axis
struct AxisDebug {
  EW_DECLARE_REFLECT
};

EW_REFLECT(AxisDebug) {
  return std::make_tuple();
}

struct CubeDebug {
  EW_DECLARE_REFLECT
};

EW_REFLECT(CubeDebug) {
  return std::make_tuple();
}
