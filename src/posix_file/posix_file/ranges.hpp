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

#ifndef POSIX_FILE_RANGES_HPP
#define POSIX_FILE_RANGES_HPP

#include <cstddef>
#include <utility>

namespace posix_file::ranges {

/**
 * An iterator for a file's offsets
 */
class offset_iterator {

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = posix_file::offset;
    using reference = value_type const&;
    using pointer = value_type const*;
    using difference_type = std::ptrdiff_t;

    constexpr offset_iterator() : m_current(0) {}

    constexpr explicit offset_iterator(posix_file::offset offset)
        : m_current(offset) {}

    // Forward iterator requirements
    // (We don't need a strictly conforming implementation since this is a
    // read only view that only returns integral values)
    constexpr value_type
    operator*() const {
        return m_current;
    }

    constexpr offset_iterator&
    operator++() {
        ++m_current;
        return *this;
    }

    constexpr offset_iterator // NOLINT
    operator++(int) {
        offset_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    // Bidirectional iterator requirements
    constexpr offset_iterator&
    operator--() {
        --m_current;
        return *this;
    }

    constexpr offset_iterator // NOLINT
    operator--(int) {
        offset_iterator tmp = *this;
        --(*this);
        return tmp;
    }

    // Random access iterator requirements
    constexpr offset_iterator&
    operator+=(difference_type n) {
        m_current += n;
        return *this;
    }

    constexpr offset_iterator
    operator+(difference_type n) const {
        return offset_iterator{m_current + n};
    }

    constexpr offset_iterator&
    operator-=(difference_type n) {
        m_current -= n;
        return *this;
    }

    constexpr offset_iterator
    operator-(difference_type n) const {
        return offset_iterator{m_current - n};
    }

    constexpr friend bool
    operator==(const offset_iterator& lhs, const offset_iterator& rhs) {
        return lhs.m_current == rhs.m_current;
    };

    friend bool
    operator!=(const offset_iterator& lhs, const offset_iterator& rhs) {
        return !(lhs == rhs);
    };

    friend constexpr difference_type
    operator-(const offset_iterator& lhs, const offset_iterator& rhs) {
        return static_cast<difference_type>(lhs.m_current - rhs.m_current);
    }

private:
    value_type m_current;
};

/**
 * A file range defined by [offset, offset+size).
 */
class range {

public:
    constexpr range(posix_file::offset offset, std::size_t size) noexcept
        : m_offset(offset), m_size(size) {}

    constexpr posix_file::offset
    offset() const noexcept {
        return static_cast<posix_file::offset>(m_offset);
    }

    constexpr std::size_t
    size() const noexcept {
        return m_size;
    }

    friend constexpr bool
    operator==(const range& lhs, const range& rhs) noexcept {
        return lhs.m_offset == rhs.m_offset && lhs.m_size == rhs.m_size;
    }

    constexpr auto
    begin() const noexcept {
        return offset_iterator{m_offset};
    }

    constexpr auto
    end() const noexcept {
        return offset_iterator{m_offset + m_size};
    }

private:
    posix_file::offset m_offset;
    std::size_t m_size;
};

template <typename Iterator>
struct iterator_range {

    using base_iterator = Iterator;

    constexpr iterator_range(Iterator&& begin_iterator, Iterator&& end_iterator)
        : m_begin_iterator(std::forward<Iterator>(begin_iterator)),
          m_end_iterator(std::forward<Iterator>(end_iterator)) {}

    constexpr std::size_t
    size() const {
        return std::distance(m_begin_iterator, m_end_iterator);
    }

    constexpr auto
    begin() const {
        return m_begin_iterator;
    }

    constexpr auto
    end() const {
        return m_end_iterator;
    }

    Iterator m_begin_iterator;
    Iterator m_end_iterator;
};

} // namespace posix_file::ranges

#ifdef POSIX_FILE_HAVE_FMT

#include <fmt/format.h>

template <>
struct fmt::formatter<posix_file::ranges::range> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const posix_file::ranges::range& r, FormatContext& ctx) const {
        const auto str =
                fmt::format("{{offset: {}, size: {}}}", r.offset(), r.size());
        return formatter<std::string_view>::format(str, ctx);
    }
};

#endif // POSIX_FILE_HAVE_FMT

#endif // POSIX_FILE_RANGES_HPP
