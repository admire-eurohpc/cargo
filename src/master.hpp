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

#ifndef CARGO_MASTER_HPP
#define CARGO_MASTER_HPP

#include "net/server.hpp"
#include "cargo.hpp"
#include "request_manager.hpp"

namespace cargo {

class master_server : public network::server,
                      public network::provider<master_server> {
public:
    master_server(std::string name, std::string address, bool daemonize,
                  std::filesystem::path rundir,
                  std::optional<std::filesystem::path> pidfile = {});

    ~master_server();

private:
    void
    mpi_listener_ult();

    void
    ping(const network::request& req);

    void
    transfer_datasets(const network::request& req,
                      const std::vector<cargo::dataset>& sources,
                      const std::vector<cargo::dataset>& targets);

    void
    transfer_status(const network::request& req, std::uint64_t tid);

private:
    // Dedicated execution stream for the MPI listener ULT
    thallium::managed<thallium::xstream> m_mpi_listener_ess;
    // ULT for the MPI listener
    thallium::managed<thallium::thread> m_mpi_listener_ult;
    // Request manager
    request_manager m_request_manager;
};

} // namespace cargo

#endif // CARGO_MASTER_HPP
