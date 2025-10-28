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

/// Checks if the type T has a reflect<T> specialization
template <class T>
concept reflectable = reflect<T>::value;

template <class T>
std::string nameOf() {
  std::string_view s;
#if defined(__GNUC__) && !defined(__clang__)
  // std::string ew::nameOf() [with T = E; std::string = std::__cxx11::basic_string<char>]
  s = __PRETTY_FUNCTION__;
  constexpr auto trim_front = 35;
  constexpr auto trim_end = 49;
#elif defined(__clang__)
  // std::string ew::nameOf() [T = E]
  s = __PRETTY_FUNCTION__;
  constexpr auto trim_front = 30;
  constexpr auto trim_end = 1;
#elif defined(_MSC_VER)
  // class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > __cdecl ew::nameOf<enum
  // E>(void)
  s = __FUNCSIG__;
  constexpr auto trim_front = 107;
  constexpr auto trim_end = 7;
#endif

  s = s.substr(trim_front, s.size() - trim_front - trim_end);

#if defined(_MSC_VER)
  if (s.starts_with("enum ")) {
    // "enum "
    s.remove_prefix(5);
  } else if (s.starts_with("struct ")) {
    // "struct "
    s.remove_prefix(7);
  } else if (s.starts_with("class ")) {
    s.remove_prefix(6);
  }
#endif

  return std::string(s);
}
} // namespace ew

#define EW_DECLARE_REFLECT \
  template <class T>       \
  friend struct ::ew::reflect;

/**
 * Used to return the reflected members of a type. This should only be used at global scope. See reflect<T> for example
 * usage.
 */
#define EW_REFLECT(T)                      \
  template <>                              \
  struct ew::reflect<T> : std::true_type { \
    static auto members();                 \
  };                                       \
  inline auto ew::reflect<T>::members()
