/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <functional>
#include <tuple>

namespace ewok::shared {
template <class T>
void hashCombine(std::size_t& seed, T const& v) {
  seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Recursive template code derived from Matthieu M.
template <class Tuple, size_t Index = std::tuple_size_v<Tuple> - 1>
struct HashValueImpl {
  static void apply(size_t& seed, Tuple const& tuple) {
    HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
    hashCombine(seed, std::get<Index>(tuple));
  }
};

template <class Tuple>
struct HashValueImpl<Tuple, 0> {
  static void apply(size_t& seed, Tuple const& tuple) {
    hashCombine(seed, std::get<0>(tuple));
  }
};
}

namespace std {
template <typename... TT>
struct hash<std::tuple<TT...>> {
  size_t operator()(std::tuple<TT...> const& tt) const {
    size_t seed = 0;
    ewok::shared::HashValueImpl<std::tuple<TT...>>::apply(seed, tt);
    return seed;
  }
};

template <typename T1, typename T2>
struct hash<std::pair<T1, T2>> {
  size_t operator()(std::pair<T1, T2> const& tt) const {
    size_t seed = 0;
    ewok::shared::hashCombine(seed, tt.first);
    ewok::shared::hashCombine(seed, tt.second);
    return seed;
  }
};
}