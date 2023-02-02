/******************************************************************************
 * Copyright 2022-2023, Barcelona Supercomputing Center (BSC), Spain
 *
 * This software was partially supported by the EuroHPC-funded project ADMIRE
 *   (Project ID: 956748, https://www.admire-eurohpc.eu).
 *
 * This file is part of Cargo.
 *
 * Cargo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cargo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cargo.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *****************************************************************************/

#ifndef POSIX_FILE_VIEWS_BLOCK_ITERATOR_HPP
#define POSIX_FILE_VIEWS_BLOCK_ITERATOR_HPP

#include "posix_file/math.hpp"
#include "posix_file/ranges.hpp"
#include <iterator>
#include <cassert>

namespace posix_file::views {

class block_iterator {

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = posix_file::ranges::range;
    using reference = value_type const&;
    using pointer = value_type const*;
    using difference_type = std::ptrdiff_t;

    constexpr block_iterator() : m_block_size(1) {}

    constexpr block_iterator(ranges::offset_iterator current,
                             ranges::offset_iterator last,
                             std::size_t block_size)
        : m_current(current), m_last(last), m_block_size(block_size) {}

    constexpr block_iterator(ranges::offset_iterator last,
                             std::size_t block_size)
        : m_current(math::align_right(*std::prev(last), block_size)),
          m_last(last), m_block_size(block_size) {}

    // Forward iterator requirements
    value_type
    operator*() const {
        assert(m_current != m_last);
        return value_type{
                *m_current,
                std::min<std::size_t>(
                        static_cast<std::size_t>(
                                std::distance(m_current, m_last)),
                        math::block_underrun(*m_current, m_block_size))};
    }

    block_iterator&
    operator++() {
        // advance m_current to the next block boundary
        const auto n = static_cast<ranges::offset_iterator::difference_type>(
                math::block_underrun(*m_current, m_block_size));
        std::advance(m_current, n);
        return *this;
    }

    block_iterator // NOLINT
    operator++(int) {
        block_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    // Bidirectional iterator requirements
    block_iterator&
    operator--() {
        // rewind m_current to the previous block boundary
        const auto n = static_cast<ranges::offset_iterator::difference_type>(
                math::block_overrun(*m_current, m_block_size));
        std::advance(m_current, -n);
        return *this;
    }

    block_iterator // NOLINT
    operator--(int) {
        block_iterator tmp = *this;
        --(*this);
        return tmp;
    }

    // Random access iterator requirements
    constexpr block_iterator&
    operator+=(difference_type n) {
        // advance m_current `n` blocks
        const auto d = static_cast<ranges::offset_iterator::difference_type>(
                math::block_underrun(*m_current, m_block_size) +
                (n - 1) * m_block_size);
        std::advance(m_current, d);
        return *this;
    }

    block_iterator
    operator+(difference_type n) const {
        const auto d = static_cast<ranges::offset_iterator::difference_type>(
                math::block_underrun(*m_current, m_block_size) +
                (n - 1) * m_block_size);
        return block_iterator{std::next(m_current, d), m_last, m_block_size};
    }

    constexpr block_iterator&
    operator-=(difference_type n) {
        // rewind m_current by `n` blocks
        const auto d = static_cast<ranges::offset_iterator::difference_type>(
                math::block_overrun(*m_current, m_block_size) +
                (n - 1) * m_block_size);
        std::advance(m_current, -d);
        return *this;
    }

    block_iterator
    operator-(difference_type n) const {
        const auto d = static_cast<ranges::offset_iterator::difference_type>(
                math::block_overrun(*m_current, m_block_size) +
                (n - 1) * m_block_size);
        return block_iterator{std::prev(m_current, d), m_last, m_block_size};
    }

    friend bool
    operator==(const block_iterator& lhs, const block_iterator& rhs) {
        return lhs.m_current == rhs.m_current;
    };

    friend bool
    operator!=(const block_iterator& lhs, const block_iterator& rhs) {
        return !(lhs == rhs);
    };

    friend constexpr difference_type
    operator-(const block_iterator& lhs, const block_iterator& rhs) {
        assert(*lhs.m_current >= *rhs.m_current);
        assert(lhs.m_block_size == rhs.m_block_size);
        return static_cast<difference_type>(
                math::block_index(*lhs.m_current, lhs.m_block_size) -
                math::block_index(*rhs.m_current, rhs.m_block_size));
    }

private:
    ranges::offset_iterator m_current;
    ranges::offset_iterator m_last;
    std::size_t m_block_size;
};

/**
 * A file view adaptor that includes a file's offsets as fixed size blocks.
 *
 * For instance, for a file `f` with `N` blocks of 512 bytes, the following
 * expression:
 *
 * ```cpp
 * posix_file f{"foo.data"};
 *
 * for(const auto& r : all_of(f) | as_blocks(512)) { ... }
 * ``
 *
 * will return the following sequence of `posix_file::range` objects if `N`
 * is larger than 1536:
 *
 * ```
 * {0, 512}, {512, 512}, {1024, 512}, ... {N*512, EOF}
 * ```
 */
class as_blocks {

public:
    /**
     * Construct an instance given a `block_size`.
     *
     * @remark `block_size` must be a power of two.
     *
     * @param block_size The block size that should be used to group file
     * offsets.
     */
    constexpr explicit as_blocks(std::size_t block_size) noexcept
        : m_block_size(block_size) {
        assert(math::is_power_of_2(block_size));
    }

    /**
     * Return the configured block size for this view.
     *
     * @return The configured block size.
     */
    constexpr std::size_t
    block_size() const noexcept {
        return m_block_size;
    }

private:
    std::size_t m_block_size;
};

template <typename Range>
constexpr auto
operator|(Range&& rng, const posix_file::views::as_blocks& a) {

    using posix_file::ranges::iterator_range;
    using posix_file::views::block_iterator;

    if(std::size(rng) == 0) {
        return iterator_range{block_iterator{}, block_iterator{}};
    }

    return iterator_range{
            block_iterator{std::begin(std::forward<Range>(rng)),
                           std::end(std::forward<Range>(rng)), a.block_size()},
            block_iterator{std::end(std::forward<Range>(rng)), a.block_size()}};
}

} // namespace posix_file::views

#endif // POSIX_FILE_VIEWS_BLOCK_ITERATOR_HPP
