/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <vector>
#include <coro/task.hpp>

namespace ew {
enum class FileError {
  FileNotFound,
  ReadFailed,
};

struct IFileProvider {
  virtual ~IFileProvider() = default;

  [[nodiscard]] virtual auto load(std::string const& fn) -> std::vector<uint8_t> = 0;

  [[nodiscard]] virtual auto loadAsync(std::string const& fn)
      -> coro::task<std::expected<std::vector<uint8_t>, FileError>> = 0;
};
using IFileProviderPtr = std::shared_ptr<IFileProvider>;
} // namespace ew
