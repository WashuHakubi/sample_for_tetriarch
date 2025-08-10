/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cmath>
#include <compare>
#include <cstdint>
#include <type_traits>

namespace ewok {
namespace detail {
template <int Start, int End, int Inc = 1, typename F>
constexpr void constexpr_for(F&& f) {
  if constexpr (Start < End) {
    f(std::integral_constant<int, Start>{});
    constexpr_for<Start + Inc, End, Inc>(f);
  }
}
}

template<class T, uint32_t N>
requires(std::is_arithmetic_v<T>)
struct VecN {
  T dim[N] = {};

  bool operator<=>(const VecN& other) const = default;
};

template <class T, uint32_t N>
constexpr VecN<T, N>& operator+=(VecN<T, N>& lhs, VecN<T, N> const& rhs) {
  detail::constexpr_for<0, N>([&](auto i) {
    lhs.dim[i] += rhs.dim[i];
  });

  return lhs;
}

template<class T, uint32_t N>
constexpr VecN<T, N> operator+(VecN<T, N> const& lhs, VecN<T, N> const& rhs) {
  auto result = lhs;
  result += rhs;
  return result;
}

template<class T, uint32_t N>
constexpr VecN<T, N>& operator-=(VecN<T, N>& lhs, VecN<T, N> const& rhs) {
  detail::constexpr_for<0, N>([&](auto i) {
    lhs.dim[i] -= rhs.dim[i];
  });

  return lhs;
}

template<class T, uint32_t N>
constexpr VecN<T, N> operator-(VecN<T, N> const& lhs, VecN<T, N> const& rhs) {
  auto result = lhs;
  result -= rhs;
  return result;
}

template<class T, uint32_t N>
constexpr VecN<T, N> operator-(VecN<T, N> const& lhs) {
  auto result = lhs;
  detail::constexpr_for<0, N>([&](auto i) {
    result.dim[i] = -result.dim[i];
  });

  return result;
}

template<class T, uint32_t N, class U = T>
constexpr VecN<T, N>& operator*=(VecN<T, N>& lhs, U const& rhs) {
  detail::constexpr_for<0, N>([&](auto i) {
    lhs.dim[i] *= rhs;
  });

  return lhs;
}

template<class T, uint32_t N, class U = T>
constexpr VecN<T, N> operator*(VecN<T, N>& lhs, U const& rhs) {
  auto result = lhs;
  result *= rhs;
  return result;
}

template<class T, uint32_t N, class U = T>
constexpr VecN<T, N>& operator/=(VecN<T, N>& lhs, U const& rhs) {
  auto const invRhs = 1 / rhs;
  detail::constexpr_for<0, N>([&](auto i) {
    lhs.dim[i] *= invRhs;
  });

  return lhs;
}

template<class T, uint32_t N>
constexpr VecN<T, N> operator/(VecN<T, N>& lhs, T const& rhs) {
  auto result = lhs;
  result /= rhs;
  return result;
}

template<class T, uint32_t N>
constexpr T dot(VecN<T, N> const& lhs, VecN<T, N> const& rhs) {
  T result = {};
  detail::constexpr_for<0, N>([&](auto i) {
    result += lhs.dim[i] * rhs.dim[i];
  });

  return result;
}

template<class T, uint32_t N>
constexpr auto sqrLength(VecN<T, N> const& lhs) {
  return dot(lhs, lhs);
}

template<class T, uint32_t N>
constexpr auto length(VecN<T, N> const& lhs) {
  return static_cast<T>(std::sqrt(sqrLength(lhs)));
}

template<class T, uint32_t N>
constexpr auto normalize(VecN<T, N> const& lhs) {
  auto const l = length(lhs);
  auto result = lhs / l;
  return result;
}

template<class T, uint32_t N>
constexpr auto sqrDistance(VecN<T, N> const& lhs, VecN<T, N> const& rhs) {
  auto l2r = lhs - rhs;
  return dot(l2r, l2r);
}

template<class T, uint32_t N>
constexpr auto distance(VecN<T, N> const& lhs, VecN<T, N> const& rhs) {
  return static_cast<T>(std::sqrt(sqrDistance(lhs, rhs)));
}

/// True if the difference between lhs and rhs is less than tolerance for all dim.
template<class T, uint32_t N, class U = T>
constexpr bool equal(VecN<T, N> const& lhs, VecN<T, N> const& rhs, U const& tolerance) {
  auto delta = lhs - rhs;
  for (auto i = 0u; i < N; ++i) {
    if (std::abs(delta.dim[i]) > tolerance) {
      return false;
    }
  }

  return true;
}

template<class T, uint32_t N>
constexpr VecN<T, 4> widen(VecN<T, N> const& lhs) {
  static_assert(N < 4, "Widen size must be less than 4");
  VecN<T, 4> result;
  detail::constexpr_for<0, N>([&](auto i) {
    result.dim[i] = lhs.dim[i];
  });
  return result;
}

using Vec2f = VecN<float, 2>;
using Vec2d = VecN<double, 2>;
using Vec2i = VecN<int32_t, 2>;
using Vec2u = VecN<uint32_t, 2>;

using Vec3f = VecN<float, 3>;
using Vec3d = VecN<double, 3>;
using Vec3i = VecN<int32_t, 3>;
using Vec3u = VecN<uint32_t, 3>;

using Vec4f = VecN<float, 4>;
using Vec4d = VecN<double, 4>;
using Vec4i = VecN<int32_t, 4>;
using Vec4u = VecN<uint32_t, 4>;
}