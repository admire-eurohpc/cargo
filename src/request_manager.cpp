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

#include "cargo.hpp"
#include "cargo/error.hpp"
#include "parallel_request.hpp"
#include "request_manager.hpp"

#include <utility>
#include "logger/logger.hpp"

namespace {} // namespace

namespace cargo {

tl::expected<parallel_request, error_code>
request_manager::create(std::size_t nfiles, std::size_t nworkers) {

    std::uint64_t tid = current_tid++;
    abt::unique_lock lock(m_mutex);

    if(const auto it = m_requests.find(tid); it == m_requests.end()) {

        const auto& [it_req, inserted] = m_requests.emplace(
                tid, std::vector<file_status>{
                             nfiles, std::vector<part_status>{nworkers}});

        if(!inserted) {
            LOGGER_ERROR("{}: Emplace failed", __FUNCTION__);
            return tl::make_unexpected(error_code::snafu);
        }
    }

    return parallel_request{tid, nfiles, nworkers};
}

error_code
request_manager::update(std::uint64_t tid, std::uint32_t seqno, std::size_t wid,
                        transfer_state s, std::optional<error_code> ec) {

    abt::unique_lock lock(m_mutex);

    if(const auto it = m_requests.find(tid); it != m_requests.end()) {
        assert(seqno < it->second.size());
        assert(wid < it->second[seqno].size());
        it->second[seqno][wid].update(s, ec);
        return error_code::success;
    }

    LOGGER_ERROR("{}: Request {} not found", __FUNCTION__, tid);
    return error_code::snafu;
}

tl::expected<request_status, error_code>
request_manager::lookup(std::uint64_t tid) {

    abt::shared_lock lock(m_mutex);

    if(const auto it = m_requests.find(tid); it != m_requests.end()) {

        const auto& file_statuses = it->second;

        for(const auto& fs : file_statuses) {
            for(const auto& ps : fs) {

                if(ps.state() == transfer_state::completed) {
                    continue;
                }

                return request_status{ps};
            }
        }

        return request_status{transfer_state::completed};
    }

    return tl::make_unexpected(error_code::snafu);
}

error_code
request_manager::remove(std::uint64_t tid) {

    abt::unique_lock lock(m_mutex);

    if(const auto it = m_requests.find(tid); it != m_requests.end()) {
        m_requests.erase(it);
        return error_code::success;
    }

    LOGGER_ERROR("{}: Request {} not found", __FUNCTION__, tid);
    return error_code::snafu;
}

} // namespace cargo