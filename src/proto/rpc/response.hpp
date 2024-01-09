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

#ifndef CARGO_PROTO_RPC_RESPONSE_HPP
#define CARGO_PROTO_RPC_RESPONSE_HPP

#include <cstdint>
#include <optional>
#include <net/serialization.hpp>

namespace cargo::proto {

template <typename Error>
class generic_response {

public:
    constexpr generic_response() noexcept = default;

    constexpr generic_response(std::uint64_t op_id, const Error& ec) noexcept
        : m_op_id(op_id), m_error_code(ec) {}

    constexpr generic_response(std::uint64_t op_id, Error&& ec) noexcept
        : m_op_id(op_id), m_error_code(std::move(ec)) {}

    [[nodiscard]] constexpr std::uint64_t
    op_id() const noexcept {
        return m_op_id;
    }

    [[nodiscard]] constexpr Error
    error_code() const noexcept {
        return m_error_code;
    }

    template <typename Archive>
    constexpr void
    serialize(Archive&& ar) {
        ar& m_op_id;
        ar& m_error_code;
    }

private:
    std::uint64_t m_op_id = 0;
    Error m_error_code{};
};

template <typename Value, typename Error>
class response_with_value : public generic_response<Error> {

public:
    constexpr response_with_value() noexcept = default;

    constexpr response_with_value(std::uint64_t op_id, const Error& ec,
                                  std::optional<Value> value) noexcept
        : generic_response<Error>(op_id, ec), m_value(std::move(value)) {}

    constexpr response_with_value(std::uint64_t op_id, Error&& ec,
                                  std::optional<Value> value) noexcept
        : generic_response<Error>(op_id, std::move(ec)),
          m_value(std::move(value)) {}

    constexpr auto
    value() const noexcept {
        return m_value.value();
    }

    constexpr auto
    has_value() const noexcept {
        return m_value.has_value();
    }

    template <typename Archive>
    constexpr void
    serialize(Archive&& ar) {
        ar(cereal::base_class<generic_response<Error>>(this), m_value);
    }

private:
    std::optional<Value> m_value;
};

template <typename Error>
using response_with_id = response_with_value<std::uint64_t, Error>;


template <typename Status, typename Bw, typename Error>
using status_response =
        response_with_value<std::tuple<Status, Bw, std::optional<Error>>,
                            Error>;

template <typename Name, typename Status, typename Bw, typename Error>
using statuses_response = response_with_value<
        std::vector<std::tuple<Name, Status, Bw, std::optional<Error>>>, Error>;

} // namespace cargo::proto

#endif // CARGO_PROTO_RPC_RESPONSE_HPP
