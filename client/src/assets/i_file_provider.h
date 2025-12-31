/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

namespace ew {
struct IFileProvider {
  virtual ~IFileProvider() = default;

  [[nodiscard]] virtual auto load(std::string const& fn) -> std::string = 0;
};
using IFileProviderPtr = std::shared_ptr<IFileProvider>;
} // namespace ew
