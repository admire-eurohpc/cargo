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
#include "ops.hpp"
#include "worker.hpp"
#include "proto/mpi/message.hpp"
#include "fmt_formatters.hpp"

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

namespace cargo {

worker::worker(int rank) : m_rank(rank) {}

int
worker::run() {

    // Create a separate communicator only for worker processes
    const mpi::communicator world;
    const auto ranks_to_exclude = std::array<int, 1>{0};
    const auto workers =
            ::make_communicator(world,
                                world.group().exclude(ranks_to_exclude.begin(),
                                                      ranks_to_exclude.end()),
                                0);

    LOGGER_INIT(fmt::format("worker_{:03}", world.rank()),
                logger::console_color);

    // Initialization finished
    LOGGER_INFO("Staging process initialized ({}:{})", world.rank(),
                workers.rank());

    bool done = false;

    while(!done) {

        auto msg = world.iprobe();

        if(!msg) {
            // FIXME: sleep time should be configurable
            std::this_thread::sleep_for(150ms);
            continue;
        }

        switch(const auto t = static_cast<tag>(msg->tag())) {
            case tag::pread:
                [[fallthrough]];
            case tag::pwrite:
                [[fallthrough]];
            case tag::sequential: {
                transfer_message m;
                world.recv(0, msg->tag(), m);
                LOGGER_CRITICAL("Transfer request received: {}", m);

                const auto op = operation::make_operation(
                        t, workers, m.input_path(), m.output_path());

                cargo::error_code ec = (*op)();

                LOGGER_EVAL(ec, INFO, ERROR, "Transfer finished: {}", ec);
                world.send(msg->source(), static_cast<int>(tag::status),
                           status_message{m.tid(), m.seqno(), ec});
                break;
            }

            default:
                LOGGER_WARN("[{}] Unexpected message tag: {}", msg->source(),
                            msg->tag());
                break;
        }
    }

    return 0;
}

} // namespace cargo
