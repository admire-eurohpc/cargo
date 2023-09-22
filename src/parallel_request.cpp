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
#include "parallel_request.hpp"

namespace cargo {

parallel_request::parallel_request(std::uint64_t tid, std::size_t nfiles,
                                   std::size_t nworkers)
    : m_tid(tid), m_nfiles(nfiles), m_nworkers(nworkers) {}

[[nodiscard]] std::uint64_t
parallel_request::tid() const {
    return m_tid;
}

[[nodiscard]] std::size_t
parallel_request::nfiles() const {
    return m_nfiles;
}

[[nodiscard]] std::size_t
parallel_request::nworkers() const {
    return m_nworkers;
}

} // namespace cargo