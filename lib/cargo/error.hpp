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

#ifndef CARGO_ERROR_HPP
#define CARGO_ERROR_HPP

#include <cstdint>
#include <string_view>

namespace cargo {

enum class error_category : std::uint32_t {
    generic_error = 0,
    system_error = 1,
    mpi_error = 2,
};

class error_code {

    enum class error_value : std::uint32_t {
        success = 0,
        snafu = 1,
        not_implemented = 2
    };

public:
    static const error_code success;
    static const error_code snafu;
    static const error_code not_implemented;

    constexpr error_code() : error_code(error_value::success) {}
    constexpr explicit error_code(error_value v)
        : m_category(error_category::generic_error), m_value(v) {}
    constexpr error_code(error_category c, std::uint32_t v)
        : m_category(c), m_value(static_cast<error_value>(v)) {}

    constexpr explicit operator bool() const {
        return m_value != error_value::success;
    }

    [[nodiscard]] constexpr error_category
    category() const {
        return m_category;
    }

    [[nodiscard]] constexpr std::uint32_t
    value() const {
        return static_cast<uint32_t>(m_value);
    }

    [[nodiscard]] constexpr std::string_view
    name() const {

        switch(m_category) {
            case error_category::generic_error:
                break;
            case error_category::system_error:
                return "CARGO_SYSTEM_ERROR";
            case error_category::mpi_error:
                return "CARGO_MPI_ERROR";
            default:
                return "CARGO_UNKNOWN_ERROR";
        }

        switch(m_value) {
            case error_value::success:
                return "CARGO_SUCCESS";
            case error_value::snafu:
                return "CARGO_SNAFU";
            case error_value::not_implemented:
                return "CARGO_NOT_IMPLEMENTED";
            default:
                return "CARGO_UNKNOWN_ERROR";
        }
    }

    [[nodiscard]] std::string
    message() const;

    template <typename Archive>
    void
    serialize(Archive&& ar, std::uint32_t version) {
        (void) version;
        ar & m_category;
        ar & m_value;
    }

private:
    error_category m_category;
    error_value m_value;
};

constexpr error_code error_code::success{error_value::success};
constexpr error_code error_code::snafu{error_value::snafu};
constexpr error_code error_code::not_implemented{error_value::not_implemented};

static constexpr cargo::error_code
make_system_error(std::uint32_t ec) {
    return cargo::error_code{cargo::error_category::system_error, ec};
}

static constexpr cargo::error_code
make_mpi_error(std::uint32_t ec) {
    return cargo::error_code{cargo::error_category::mpi_error, ec};
}

constexpr bool
operator==(const error_code& lhs, const error_code& rhs) {
    return lhs.category() == rhs.category() && lhs.value() == rhs.value();
}

} // namespace cargo

#endif // CARGO_ERROR_HPP
