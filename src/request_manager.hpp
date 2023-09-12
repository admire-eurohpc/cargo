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

#ifndef CARGO_REQUEST_MANAGER_HPP
#define CARGO_REQUEST_MANAGER_HPP

#include <tl/expected.hpp>
#include <atomic>
#include "request.hpp"
#include "shared_mutex.hpp"

namespace cargo {

class dataset;

/**
 * A manager for transfer requests.
 *
 * A single transfer requests may involve `N` files and each file may
 * be served by `W` MPI workers. Thus, the manager keeps a map of request IDs
 * to a vector of `N` `file_status`es, where each element is in turn also
 * a vector with `W` `part_status` values, one for each worker in charge of
 * processing a particular file region.
 *
 * For example:
 *   request 42 -> file_status[0] -> worker [0] -> pending
 *                                   worker [1] -> pending
 *              -> file_status[1] -> worker [0] -> complete
 *                                   worker [1] -> complete
 *                                   worker [2] -> running
 */
class request_manager {

    using file_status = std::vector<part_status>;

public:
    tl::expected<request, error_code>
    create(std::size_t nfiles, std::size_t nworkers);

    tl::expected<void, error_code>
    update(std::uint64_t tid, std::uint32_t seqno, std::size_t wid,
           part_status status);

    tl::expected<request_status, error_code>
    lookup(std::uint64_t tid);

    tl::expected<void, error_code>
    remove(std::uint64_t tid);

private:
    std::atomic<std::uint64_t> current_tid = 0;
    mutable abt::shared_mutex m_mutex;
    std::unordered_map<std::uint64_t, std::vector<file_status>> m_requests;
};

} // namespace cargo

#endif // CARGO_REQUEST_MANAGER_HPP
