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

#ifndef CARGO_PARALLEL_REQUEST_HPP
#define CARGO_PARALLEL_REQUEST_HPP

#include <cstdint>
#include <vector>
#include <optional>
#include <fmt/format.h>

namespace cargo {

class dataset;

class parallel_request {

public:
    parallel_request(std::uint64_t id, std::size_t nfiles,
                     std::size_t nworkers);

    [[nodiscard]] std::uint64_t
    tid() const;

    [[nodiscard]] std::size_t
    nfiles() const;

    [[nodiscard]] std::size_t
    nworkers() const;

private:
    /** Unique identifier for the request */
    std::uint64_t m_tid;
    /** Number of files to be processed by the request */
    std::size_t m_nfiles;
    /** Number of workers to be used for the request */
    std::size_t m_nworkers;
};

/**
 * The status of a single file part.
 */
class part_status {
public:
    part_status() = default;

    [[nodiscard]] transfer_state
    state() const;

    [[nodiscard]] std::optional<error_code>
    error() const;

    void
    update(transfer_state s, std::optional<error_code> ec) noexcept;

private:
    transfer_state m_state{transfer_state::pending};
    std::optional<error_code> m_error_code{};
};

class request_status {
public:
    request_status() = default;
    explicit request_status(transfer_state s,
                            std::optional<error_code> ec = {});
    explicit request_status(part_status s);

    [[nodiscard]] transfer_state
    state() const;

    [[nodiscard]] std::optional<error_code>
    error() const;

private:
    transfer_state m_state{transfer_state::pending};
    std::optional<error_code> m_error_code{};
};

} // namespace cargo

template <>
struct fmt::formatter<cargo::request_status> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::request_status& s, FormatContext& ctx) const {

        const auto state_name = [](auto&& s) {
            switch(s.state()) {
                case cargo::transfer_state::pending:
                    return "pending";
                case cargo::transfer_state::running:
                    return "running";
                case cargo::transfer_state::completed:
                    return "completed";
                case cargo::transfer_state::failed:
                    return "failed";
                default:
                    return "unknown";
            }
        };

        const auto str = fmt::format("{{state: {}, error_code: {}}}",
                                     state_name(s), s.error());
        return formatter<std::string_view>::format(str, ctx);
    }
};

#endif // CARGO_PARALLEL_REQUEST_HPP
