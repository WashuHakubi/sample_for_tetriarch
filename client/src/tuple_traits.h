/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <tuple>

namespace ew {
namespace detail {
template <class Tuple, class Fn, std::size_t I, std::size_t N>
void visit_each_impl(Tuple&& t, Fn&& fn) {
  if constexpr (I < N) {
    fn(std::get<I>(t));
    visit_each_impl<Tuple, Fn, I + 1, N>(std::forward<Tuple>(t), std::forward<Fn>(fn));
  }
}
} // namespace detail

template <class Tuple, class Fn>
void visit_each(Tuple&& t, Fn&& fn) {
  detail::visit_each_impl<Tuple, Fn, 0, std::tuple_size_v<std::decay_t<Tuple>>>(
      std::forward<Tuple>(t),
      std::forward<Fn>(fn));
}
} // namespace ew