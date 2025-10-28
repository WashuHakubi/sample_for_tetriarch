/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once
#include <functional>
#include <tuple>
#include <type_traits>

namespace ew {
namespace detail {
template <class TRet, class... TArgs>
struct FnSigBase {
  using Sig = TRet(TArgs...);
  using Function = std::function<Sig>;
  using ArgTuple = std::tuple<TArgs...>;
  static constexpr size_t argCount = std::tuple_size_v<ArgTuple>;
};

template <class T, typename = void>
struct FnSig;

// Function
template <class TRet, class... TArgs>
struct FnSig<TRet(TArgs...)> : FnSigBase<TRet, TArgs...> {};

// Member function, used by lambda callable
template <class T, class TRet, class... TArgs>
struct FnSig<TRet (T::*)(TArgs...)> : FnSigBase<TRet, TArgs...> {};

// Const member function, used by lambda callable
template <class T, class TRet, class... TArgs>
struct FnSig<TRet (T::*)(TArgs...) const> : FnSigBase<TRet, TArgs...> {};

// Lambda callables
template <class T>
struct FnSig<T, decltype(void(&T::operator()))> : FnSig<decltype(&T::operator())> {};

template <class ArgTuple, size_t I>
struct FnArgTraits {
  using Type = std::tuple_element_t<I, ArgTuple>;
  using ElementType = std::decay_t<std::remove_pointer_t<Type>>;

  static constexpr bool isReadOnly = std::is_const_v<std::remove_pointer_t<std::remove_reference_t<Type>>>;
  static constexpr bool isOptional = std::is_pointer_v<Type>;

  static_assert(isReadOnly || !isOptional, "Optional components must be read only.");
};

template <class T>
void hashCombine(std::size_t& seed, const T& v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
} // namespace detail
} // namespace ew