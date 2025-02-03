/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <string>
#include <string_view>

namespace ewok {
template <typename T>
std::string getTypeName() {
#if defined(__clang__) || defined(__GNUC__)
  char const* fnName = __PRETTY_FUNCTION__;
  const size_t offset = 37;
  const size_t trim = 1;
#elif defined(_MSC_VER)
  char const* fnName = __FUNCSIG__;
  const size_t offset = 114;
  const size_t trim = 7;
#else
#error Unknown compoiler for getTypeName()
#endif
  auto p = std::string_view(fnName).substr(offset);
  p = p.substr(0, p.size() - trim);
#if defined(_MSC_VER)
  if (p.starts_with("struct ")) {
    p = p.substr(7);
  }
  if (p.starts_with("class ")) {
    p = p.substr(6);
  }
#endif
  return std::string(p.begin(), p.end());
}
} // namespace ewok