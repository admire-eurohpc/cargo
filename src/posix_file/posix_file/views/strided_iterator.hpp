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


#ifndef POSIX_FILE_VIEWS_STRIDED_ITERATOR_HPP
#define POSIX_FILE_VIEWS_STRIDED_ITERATOR_HPP

#include <cstdlib>
#include <algorithm>
#include <iterator>

namespace posix_file::views {

template <typename BaseIterator>
class strided_iterator {

    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename BaseIterator::value_type;
    using reference = value_type const&;
    using pointer = value_type const*;
    using difference_type = typename BaseIterator::difference_type;

public:
    constexpr strided_iterator() = default;
    constexpr strided_iterator(BaseIterator first, BaseIterator last,
                               difference_type step, difference_type disp)
        : m_first(first), m_current(first), m_last(last), m_step(step),
          m_disp(disp) {

        if(m_disp != 0) {
            std::advance(m_current,
                         std::min(std::abs(std::distance(m_current, m_last)),
                                  m_disp));
        }
    }

    constexpr value_type
    operator*() {
        return *m_current;
    }

    constexpr strided_iterator&
    operator++() {
        std::advance(
                m_current,
                std::min(std::abs(std::distance(m_current, m_last)), m_step));
        return *this;
    }

    constexpr strided_iterator // NOLINT
    operator++(int) {
        strided_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr strided_iterator&
    operator--() {
        std::advance(
                m_current,
                -std::min(std::abs(std::distance(m_first, m_current)), m_step));
        return *this;
    }

    constexpr strided_iterator // NOLINT
    operator--(int) {
        strided_iterator tmp = *this;
        --(*this);
        return tmp;
    }

    constexpr friend bool
    operator==(const strided_iterator& lhs, const strided_iterator& rhs) {
        return lhs.m_current == rhs.m_current;
    };

    constexpr friend bool
    operator!=(const strided_iterator& lhs, const strided_iterator& rhs) {
        return !(lhs == rhs);
    };

    BaseIterator m_first;
    BaseIterator m_current;
    BaseIterator m_last;
    difference_type m_step = 0;
    difference_type m_disp = 0;
};

/**
 * An adaptor view including elements from another view advancing over N
 * elements at a time and (optionally) skipping D elements from its beginning.
 *
 * For instance, for a file `f` with `N` blocks of 512 bytes, the following
 * expression:
 *
 * ```cpp
 * posix_file f{"foo.data"};
 *
 * for(const auto& r : all_of(f) | as_blocks(512) | strided(2) { ... }
 * ``
 *
 * will return the following sequence of `posix_file::range` objects if `N`
 * is even and larger than 2560:
 *
 * ```
 * {0, 512}, {1024, 512}, {2048, 512}, ... {N*512, EOF}
 * ```
 */
class strided {

public:
    /**
     * Construct an instance of the view given a `step` and (optionally) an
     * initial displacement `skip`.
     *
     * @param step The number of elements that should be advanced.
     * @param skip The number of elements in the beginning of the sequence that
     * should be skipped.
     */
    constexpr explicit strided(std::size_t step, std::size_t skip = 0) noexcept
        : m_step(step), m_skip(skip) {}

    /**
     * Return the step size for this view.
     *
     * @return The configured step size.
     */
    constexpr std::size_t
    step() const noexcept {
        return m_step;
    }

    /**
     * Return the number of elements that this view should skip.
     *
     * @return The number of elements that should be skipped.
     */
    constexpr std::size_t
    skip() const {
        return m_skip;
    }

private:
    std::size_t m_step;
    std::size_t m_skip;
};

template <typename Range>
constexpr auto
operator|(Range&& rng, const posix_file::views::strided& s) {

    using posix_file::views::strided_iterator;
    using ranges::iterator_range;

    using BaseIterator = typename Range::base_iterator;
    using difference_type = typename BaseIterator::difference_type;

    if(std::size(rng) == 0) {
        return iterator_range{strided_iterator<BaseIterator>{},
                              strided_iterator<BaseIterator>{}};
    }

    return iterator_range{
            strided_iterator{std::begin(std::forward<Range>(rng)),
                             std::end(std::forward<Range>(rng)),
                             static_cast<difference_type>(s.step()),
                             static_cast<difference_type>(s.skip())},
            strided_iterator{std::end(std::forward<Range>(rng)),
                             std::end(std::forward<Range>(rng)),
                             static_cast<difference_type>(s.step()),
                             static_cast<difference_type>(s.skip())}};
}

} // namespace posix_file::views

#endif // POSIX_FILE_VIEWS_STRIDED_ITERATOR_HPP
