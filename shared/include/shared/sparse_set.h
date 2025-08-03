/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <cassert>
#include <cstdint>
#include <vector>

namespace ewok::shared {
enum class Entity : uint32_t {
};

class SparseSetIterator {
public:
  using dense_container = std::vector<Entity>;
  using value_type = typename dense_container::value_type;
  using pointer = typename dense_container::const_pointer;
  using reference = typename dense_container::const_reference;
  using difference_type = typename dense_container::difference_type;
  using iterator_category = std::random_access_iterator_tag;

  constexpr SparseSetIterator() = default;

  constexpr SparseSetIterator(dense_container const& c, size_t offset) : c_(&c), offset_(offset) {}

  constexpr SparseSetIterator &operator++() noexcept {
    --offset_;
    return *this;
  }

  constexpr SparseSetIterator operator++(int) noexcept {
    auto const orig = *this;
    --offset_;
    return orig;
  }

  constexpr SparseSetIterator &operator--() noexcept {
    ++offset_;
    return *this;
  }

  constexpr SparseSetIterator operator--(int) noexcept {
    auto const orig = *this;
    ++offset_;
    return orig;
  }

  constexpr SparseSetIterator &operator+=(const difference_type value) noexcept {
    offset_ -= value;
    return *this;
  }

  constexpr SparseSetIterator operator+(const difference_type value) const noexcept {
    auto copy = *this;
    return (copy += value);
  }

  constexpr SparseSetIterator &operator-=(const difference_type value) noexcept {
    return *this += -value;
  }

  constexpr SparseSetIterator operator-(const difference_type value) const noexcept {
    return *this + -value;
  }

  [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
    return (*c_)[static_cast<typename dense_container::size_type>(index() - value)];
  }

  [[nodiscard]] constexpr pointer operator->() const noexcept {
    return std::addressof(operator[](0));
  }

  [[nodiscard]] constexpr reference operator*() const noexcept {
    return operator[](0);
  }

  [[nodiscard]] constexpr pointer data() const noexcept {
    return c_ ? c_->data() : nullptr;
  }

  [[nodiscard]] constexpr difference_type index() const noexcept {
    return offset_ - 1;
  }

private:
  dense_container const* c_{};
  size_t offset_{};
};

[[nodiscard]] constexpr std::ptrdiff_t operator-(SparseSetIterator const &lhs, SparseSetIterator const &rhs) noexcept {
  return rhs.index() - lhs.index();
}

[[nodiscard]] constexpr bool operator==(SparseSetIterator const &lhs, SparseSetIterator const &rhs) noexcept {
  return lhs.index() == rhs.index();
}

[[nodiscard]] constexpr bool operator!=(SparseSetIterator const &lhs, SparseSetIterator const &rhs) noexcept {
  return !(lhs == rhs);
}

[[nodiscard]] constexpr bool operator<(SparseSetIterator const &lhs, SparseSetIterator const &rhs) noexcept {
  return lhs.index() > rhs.index();
}

[[nodiscard]] constexpr bool operator>(SparseSetIterator const &lhs, SparseSetIterator const &rhs) noexcept {
  return rhs < lhs;
}

[[nodiscard]] constexpr bool operator<=(SparseSetIterator const &lhs, SparseSetIterator const &rhs) noexcept {
  return !(lhs > rhs);
}

[[nodiscard]] constexpr bool operator>=(SparseSetIterator const &lhs, SparseSetIterator const &rhs) noexcept {
  return !(lhs < rhs);
}

class SparseSet {
  static constexpr uint32_t toIntegral(Entity e) noexcept {
    return static_cast<uint32_t>(e);
  }

  static constexpr Entity toEntity(uint32_t v) noexcept {
    return static_cast<Entity>(v);
  }

  static constexpr size_t PageSize = 4096;
  static constexpr uint32_t Tombstone = UINT32_MAX;

  using sparse_container = std::vector<std::unique_ptr<uint32_t[]>>;
  using dense_container = std::vector<Entity>;

public:
  using size_type = dense_container::size_type;
  using difference_type = dense_container::difference_type;

  using value_type = dense_container::value_type;
  using reference = dense_container::reference;
  using const_reference = dense_container::const_reference;
  using pointer = dense_container::pointer;
  using const_pointer = dense_container::const_pointer;

  using iterator = SparseSetIterator;
  using const_iterator = SparseSetIterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  /// Gets the page for an index
  [[nodiscard]] static auto toPage(const size_type v) noexcept {
    return v / PageSize;
  }
  
  /// Gets a pointer to the sparse entry for an entity, or nullptr if the page does not exist.
  [[nodiscard]] auto* sparsePtr(const value_type v) const noexcept {
    auto const pos = toIntegral(v);
    auto const pageIdx = toPage(pos);
    auto const indexInPage = pos % PageSize;

    return sparse_.size() > pageIdx && sparse_[pageIdx] ? sparse_[pageIdx].get() + indexInPage : nullptr;
  }

  /// Gets a reference to the sparse entry for an entity. The page must exist.
  [[nodiscard]] auto& sparseRef(const value_type v) const noexcept {
    auto p = sparsePtr(v);
    assert(p);
    return *p;
  }

  [[nodiscard]] iterator toIterator(const value_type v) const noexcept {
    return toIterator(index(v));
  }

  [[nodiscard]] iterator toIterator(const size_type v) const noexcept {
    // Since we traverse the dense container backwards end() actually points to the first element of dense. Since the
    // iterator moves in the opposite direction, then subtraction is equivalent to addition. We then add one more to it
    // via pre-increment to move it one past the position.
    return --(end() - static_cast<difference_type>(v));
  }

  /// Grows the sparse container to hold the page for the entity @c v
  /// @returns A reference to the sparse entry
  auto& growToContain(const value_type v) {
    auto const pos = toIntegral(v);
    auto const pageIdx = toPage(pos);
    auto const indexInPage = pos % PageSize;

    if (pageIdx >= sparse_.size()) {
      // Ensure we contain the page
      sparse_.resize(pageIdx + 1);
    }

    if (auto& p = sparse_[pageIdx]; p == nullptr) {
      // If the page is null, create it
      p = std::make_unique<uint32_t[]>(PageSize);
      // Initialize the array to contain our tombstone value
      std::uninitialized_fill_n(p.get(), PageSize, Tombstone);
    }

    return sparse_[pageIdx][indexInPage];
  }

  void swapItems(const value_type lhs, const value_type rhs) noexcept {
    auto& lPos = sparseRef(lhs);
    auto& rPos = sparseRef(rhs);

    std::swap(dense_[lPos], dense_[rPos]);
    std::swap(lPos, rPos);
  }

public:
  [[nodiscard]] iterator begin() const noexcept {
    return {dense_, dense_.size()};
  }

  [[nodiscard]] iterator end() const noexcept {
    return {dense_, 0};
  }

  [[nodiscard]] const_iterator cbegin() const noexcept {
    return {dense_, dense_.size()};
  }

  [[nodiscard]] const_iterator cend() const noexcept {
    return {dense_, 0}; }

  [[nodiscard]] size_type size() const noexcept {
    return dense_.size();
  }

  [[nodiscard]] bool contains(const value_type v) const noexcept {
    // Check the sparse table to see if the entry is allocated. As long as it is not a tombstone, then it should exist in
    // the dense set.
    auto p = sparsePtr(v);
    assert(!p || *p == Tombstone || *p < dense_.size());
    return p && *p != Tombstone;
  }

  [[nodiscard]] iterator find(const value_type v) const noexcept {
    auto p = sparsePtr(v);
    if (!p || *p == Tombstone) {
      return end();
    }

    return toIterator(*p);
  }

  [[nodiscard]] size_type index(const value_type v) const noexcept {
    assert(contains(v));
    return sparseRef(v);
  }

  iterator insert(value_type v) {
    auto idx = dense_.size();
    dense_.push_back(v);
    auto &p = growToContain(v);
    p = static_cast<uint32_t>(idx);

    return toIterator(idx);
  }

  iterator erase(value_type v) {
    auto idx = index(v);

    // Swap with the last item and then remove the last item.
    swapItems(dense_.back(), v);
    dense_.pop_back();

    // Update the entry in the sparse table to indicate that it is no longer allocated.
    sparseRef(v) = Tombstone;

    // And return an iterator to the element _after_ this one, which might be end().
    return toIterator(idx - 1);
  }
private:
  sparse_container sparse_;
  dense_container dense_;
};
}