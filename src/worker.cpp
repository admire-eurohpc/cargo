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

#include <fmt/format.h>
#include <logger/logger.hpp>
#include <boost/mpi.hpp>
#include <boost/mpi/error_string.hpp>
#include "message.hpp"

namespace mpi = boost::mpi;
using namespace std::chrono_literals;


namespace {

// boost MPI doesn't have a communicator constructor that uses
// MPI_Comm_create_group()
mpi::communicator
make_communicator(const mpi::communicator& comm, const mpi::group& group,
                  int tag) {
    MPI_Comm newcomm;
    if(const auto ec = MPI_Comm_create_group(comm, group, tag, &newcomm);
       ec != MPI_SUCCESS) {
        LOGGER_ERROR("MPI_Comm_create_group() failed: {}",
                     boost::mpi::error_string(ec));
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    return mpi::communicator{newcomm, boost::mpi::comm_take_ownership};
}


} // namespace

void
mpio_read(const mpi::communicator& workers,
          const std::filesystem::path& input_path,
          const std::filesystem::path& output_path) {

    (void) workers;
    (void) input_path;
    (void) output_path;

    LOGGER_CRITICAL("{}: to be implemented", __FUNCTION__);
}

void
mpio_write(const mpi::communicator& workers,
           const std::filesystem::path& input_path,
           const std::filesystem::path& output_path) {

    (void) workers;
    (void) input_path;
    (void) output_path;

    LOGGER_CRITICAL("{}: to be implemented", __FUNCTION__);
}

void
sequential_transfer(const std::filesystem::path& input_path,
                    const std::filesystem::path& output_path) {
    (void) input_path;
    (void) output_path;

    LOGGER_CRITICAL("{}: to be implemented", __FUNCTION__);
}

void
worker() {

    // Create a separate communicator only for worker processes
    const mpi::communicator world;
    const auto ranks_to_exclude = std::array<int, 1>{0};
    const auto workers =
            ::make_communicator(world,
                                world.group().exclude(ranks_to_exclude.begin(),
                                                      ranks_to_exclude.end()),
                                0);

    LOGGER_INIT(fmt::format("worker_{:03}", world.rank()), "console color");

    // Initialization finished
    LOGGER_INFO("Staging process initialized (world_rank {}, workers_rank: {})",
                world.rank(), workers.rank());

    bool done = false;

    while(!done) {

        auto msg = world.iprobe();

        if(!msg) {
            // FIXME: sleep time should be configurable
            std::this_thread::sleep_for(150ms);
            continue;
        }

        switch(static_cast<cargo::message_tags>(msg->tag())) {
            case cargo::message_tags::transfer: {
                cargo::transfer_request_message m;
                world.recv(mpi::any_source, msg->tag(), m);
                LOGGER_DEBUG("Transfer request received!: {}", m);

                switch(m.type()) {
                    case cargo::parallel_read:
                        ::mpio_read(workers, m.input_path(), m.output_path());
                        break;
                    case cargo::parallel_write:
                        ::mpio_write(workers, m.input_path(), m.output_path());
                        break;
                    case cargo::sequential:
                        ::sequential_transfer(m.input_path(), m.output_path());
                        break;
                }
                break;
            }

            case cargo::message_tags::status: {
                cargo::transfer_status_message m;
                world.recv(mpi::any_source, msg->tag(), m);
                LOGGER_DEBUG("Transfer status query received!: {}", m);
                break;
            }

            case cargo::message_tags::shutdown:
                done = true;
                break;
        }
    }
}
