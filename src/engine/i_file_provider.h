/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/forward.h"

namespace ewok {
struct IFileProvider {
  virtual ~IFileProvider() = default;

  virtual auto readFileAsync(
      std::string const& fn,
      std::shared_ptr<concurrencpp::executor> const& resumeOnExecutor)
      -> concurrencpp::result<std::vector<char>> = 0;

  virtual auto blockingReadFile(std::string const& fn)
      -> std::expected<std::vector<char>, std::error_code> = 0;
};
} // namespace ewok
