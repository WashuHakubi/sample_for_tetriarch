/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "engine/null_file_provider.h"

namespace ewok {
auto NullFileProvider::instance() -> std::shared_ptr<NullFileProvider> const& {
  static std::shared_ptr<NullFileProvider> inst =
      std::make_shared<NullFileProvider>();
  return inst;
}

auto NullFileProvider::readFileAsync(
    std::string const& fn,
    std::shared_ptr<concurrencpp::executor> const& resumeOnExecutor)
    -> concurrencpp::result<std::vector<char>> {
  co_return std::vector<char>{};
}

auto NullFileProvider::blockingReadFile(std::string const& fn)
    -> std::expected<std::vector<char>, std::error_code> {
  return std::vector<char>{};
}
} // namespace ewok
