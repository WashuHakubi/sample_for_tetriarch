/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include "engine/i_file_provider.h"

namespace ewok {
class SystemFileProvider : public IFileProvider {
 public:
  SystemFileProvider(
      std::shared_ptr<concurrencpp::executor> ioExecutor, std::string path);

  auto readFileAsync(
      std::string const& fn,
      std::shared_ptr<concurrencpp::executor> const& resumeOnExecutor)
      -> concurrencpp::result<std::vector<char>> override;

 private:
  auto ioReadFileAsync(std::string const& fn) -> concurrencpp::result<
      std::expected<std::vector<char>, std::error_code>>;

  std::shared_ptr<concurrencpp::executor> ioExecutor_;
  std::string path_;
};
} // namespace ewok
