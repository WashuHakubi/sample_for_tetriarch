/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <concepts>
#include <format>
#include <span>
#include <string>
#include <vector>

namespace ew::attrs {
struct errors {
  void append(std::string_view name, std::string msg) {
    if (msg.empty()) {
      return;
    }
    errors_.push_back(std::format("{}: {}", name, msg));
  }

  bool success() { return errors_.empty(); }

  std::span<std::string const> items() const { return errors_; }

 private:
  std::vector<std::string> errors_;
};

template <class T, class U>
concept validatable = requires(T t, errors e, std::string_view path, U u) {
  { t.validate(e, path, u) };
};

/// Indicates that serialization of this type should consider compression if available.
[[maybe_unused]] constexpr struct compress_tag {
} compress;

/// Indicates the allowable range for some type. Min and max are inclusive by default.
template <class T, bool InclusiveMin = true, bool InclusiveMax = true>
struct allowed_range {
  T min;
  T max;

  void validate(errors& err, std::string_view path, T value) const {
    if ((InclusiveMin && value < min) || (!InclusiveMin && value <= min)) {
      err.append(
          path,
          std::format("value {} was below min value {}{}", value, min, InclusiveMin ? "" : " (exclusive)"));
    }
    if ((InclusiveMax && value > max) || (!InclusiveMax && value >= max)) {
      err.append(
          path,
          std::format("value {} was above max value {}{}", value, max, InclusiveMax ? "" : " (exclusive)"));
    }
  }
};
} // namespace ew::attrs
