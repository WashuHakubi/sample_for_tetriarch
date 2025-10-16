/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <tuple>
#include <type_traits>

namespace ew {
namespace detail {
template <class T, class Tuple>
struct contains;

template <class T>
struct contains<T, std::tuple<>> : std::false_type {};

template <class T, class U, class... Rest>
struct contains<T, std::tuple<U, Rest...>> : contains<T, std::tuple<Rest...>> {};

template <class T, class... Rest>
struct contains<T, std::tuple<T, Rest...>> : std::true_type {};

template <class F, class Tuple, size_t I, size_t N>
void apply_impl(F&& f, Tuple&& t) {
  if constexpr (I < N) {
    f(std::get<I>(t));
    apply_impl<F, Tuple, I + 1, N>(std::forward<F>(f), std::forward<Tuple>(t));
  }
}
} // namespace detail

/// True if Tuple contains T
template <class T, class Tuple>
constexpr bool contains_v = detail::contains<T, Tuple>::value;

/// Applies <c>f</c> to each element of tuple <c>t</c> as <c>f(std::get<I>(t))</c>
template <class F, class Tuple>
void apply(F&& f, Tuple&& t) {
  detail::apply_impl<F, Tuple, 0, std::tuple_size_v<std::decay_t<Tuple>>>(std::forward<F>(f), std::forward<Tuple>(t));
}
} // namespace ew
