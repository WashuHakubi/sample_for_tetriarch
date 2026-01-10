/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once
#include "i_file_provider.h"

#include <coro/io_scheduler.hpp>

struct SimpleFileProvider : ew::IFileProvider {
  explicit SimpleFileProvider(std::shared_ptr<coro::io_scheduler> scheduler, std::string basePath);

  [[nodiscard]] auto load(std::string const& fn) -> std::vector<uint8_t> override;

  [[nodiscard]] auto loadAsync(std::string const& fn)
      -> coro::task<std::expected<std::vector<uint8_t>, ew::FileError>> override;

 private:
  std::string basePath_;
  std::shared_ptr<coro::io_scheduler> scheduler_;
};
