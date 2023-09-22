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

enum class part_status { pending, running, completed, failed };
enum class request_status { pending, running, completed, failed };

} // namespace cargo

#endif // CARGO_PARALLEL_REQUEST_HPP
