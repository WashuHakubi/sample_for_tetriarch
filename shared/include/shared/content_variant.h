/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#include <variant>

#include <shared/serialization.h>

namespace ewok::shared {
///
template <class TDiscriminator, class... Ts>
struct ContentVariant : std::variant<Ts...> {
  auto type() const -> TDiscriminator {
    return static_cast<TDiscriminator>(this->index());
  }
};
}

template <class TDiscriminator, class... Ts>
struct ewok::shared::serialization::CustomSerializable<
      ewok::shared::ContentVariant<TDiscriminator, Ts...>> : std::true_type {
  using value_type = ContentVariant<TDiscriminator, Ts...>;
  using variant_type = std::variant<Ts...>;

  static constexpr auto serialize(TSerializeWriter auto& writer, value_type const& value) {
    // Write out the type of the variant...
    if (auto r = detail::serializeItem(writer, "type", value.type()); !r) {
      return r;
    }

    // Then write out the value in the variant.
    return std::visit(
        value,
        [&writer](auto const& v) {
          auto r = detail::serializeItem(writer, "value", v);
          return r;
        });
  }

  static constexpr auto deserialize(TSerializeReader auto& reader, value_type& value) {
    // Read in the type we expect
    TDiscriminator type;
    if (auto r = detail::deserializeItem(reader, "type", type); !r) {
      return r;
    }

    // Recursively check if the type at the index matches the type we read, if it does then load that into the variant
    return readIfExpectedType<std::variant_size_v<variant_type>, 0>(reader, type, value);
  }

private:
  template <size_t N, size_t I>
  static auto readIfExpectedType(
      TSerializeReader auto& reader,
      TDiscriminator expected,
      value_type& value) -> Result {
    // Only read in a value if we're still withing the bounds of the variant.
    if constexpr (I < N) {
      // Check if I is the type we want
      if (static_cast<TDiscriminator>(I) == expected) {
        // If it is then read that value
        std::variant_alternative_t<I, variant_type> v;
        if (auto r = detail::deserializeItem(reader, "value", v); !r) {
          return r;
        }

        // And set it in the variant.
        value = v;
        return {};
      }

      // Otherwise try the next type in the variant.
      return readIfExpectedType<N, I + 1>(reader, expected, value);
    }

    // Past the end of the variant, something went wrong.
    return std::unexpected{Error::InvalidFormat};
  }
};
