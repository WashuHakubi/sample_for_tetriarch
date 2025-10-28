/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <shared/attrs.h>
#include <shared/type_traits.h>

namespace ew {
/**
 * Template class for getting reflection members. This class should be specialized for each type T whose members should
 * be reflected. For example,
 * <code>
 * struct S {
 *   float f;
 *   int i;
 *
 *   EW_DECLARE_REFLECT;
 * };
 *
 * EW_REFLECT(T) {
 *   return std::make_tuple(
 *     std::make_tuple("f", &S::f),
 *     std::make_tuple("i", &S::i, ew::attrs::compress);
 * }
 * </code>
 */
template <class T>
struct reflect : std::false_type {};
} // namespace ew

#define EW_DECLARE_REFLECT \
  template <class T>       \
  friend struct ::ew::reflect;

/**
 * Used to return the reflected members of a type. This should only be used at global scope. See reflect<T> for example
 * usage.
 */
#define EW_REFLECT(T)                        \
  template <>                                \
  struct ::ew::reflect<T> : std::true_type { \
    static auto members();                   \
  };                                         \
  inline auto ::ew::reflect<T>::members()