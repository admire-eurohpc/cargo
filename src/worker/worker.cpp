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

#include <thread>
#include <fmt/format.h>
#include <logger/logger.hpp>
#include <boost/mpi.hpp>
#include <boost/mpi/error_string.hpp>

#include "worker.hpp"
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

void
update_state(int rank, std::uint64_t tid, std::uint32_t seqno,
             cargo::transfer_state st, float bw,
             std::optional<cargo::error_code> ec = std::nullopt) {

    mpi::communicator world;
    const cargo::status_message m{tid, seqno, st, bw, ec};
    LOGGER_DEBUG("msg <= to: {} body: {{payload: {}}}", rank, m);
    world.send(rank, static_cast<int>(cargo::tag::status), m);
}

} // namespace

namespace cargo {

worker::worker(std::string name, int rank)
    : m_name(std::move(name)), m_rank(rank) {}

void
worker::set_output_file(std::filesystem::path output_file) {
    m_output_file = std::move(output_file);
}

void
worker::set_block_size(std::uint64_t block_size) {
    m_block_size = block_size;
}

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

    const logger::logger_config cfg{
            fmt::format("{}:{:03}", m_name, world.rank()),
            m_output_file ? logger::file : logger::console_color,
            m_output_file};

    logger::create_default_logger(cfg);

    const auto greeting =
            fmt::format("Starting staging process (pid {})", getpid());

    LOGGER_INFO("{:=>{}}", "", greeting.size());
    LOGGER_INFO(greeting);
    LOGGER_INFO("{:=>{}}", "", greeting.size());

    bool done = false;
    while(!done) {
        // Always loop pending operations

        auto I = m_ops.begin();
        auto IE = m_ops.end();
        if(I != IE) {
            auto op = I->second.first.get();
            int index = I->second.second;
            if(op) {
                index = op->progress(index);
                if(index == -1) {
                    // operation finished
                    cargo::error_code ec = op->progress();
                    update_state(op->source(), op->tid(), op->seqno(),
                                 ec ? transfer_state::failed
                                    : transfer_state::completed,
                                 0.0f, ec);

                    // Transfer finished
                    I = m_ops.erase(I);
                } else {
                    // update only if BW is set
                    if(op->bw() > 0.0f) {
                        update_state(op->source(), op->tid(), op->seqno(),
                                     transfer_state::running, op->bw());
                    }
                    I->second.second = index;
                    ++I;
                }
            }
        }


        auto msg = world.iprobe();

        if(!msg) {
            // Only wait if there are no pending operations and no messages
            if(m_ops.size() == 0) {
                std::this_thread::sleep_for(150ms);
            }
            continue;
        }

        switch(const auto t = static_cast<tag>(msg->tag())) {
            case tag::pread:
                [[fallthrough]];
            case tag::pwrite:
                [[fallthrough]];
            case tag::sequential: {
                transfer_message m;
                world.recv(msg->source(), msg->tag(), m);
                LOGGER_INFO("msg => from: {} body: {}", msg->source(), m);
                m_ops.emplace(std::make_pair(
                        make_pair(m.input_path(), m.output_path()),
                        make_pair(operation::make_operation(
                                          t, workers, m.input_path(),
                                          m.output_path(), m_block_size, m.type()),
                                  0)));

                const auto op =
                        m_ops[make_pair(m.input_path(), m.output_path())]
                                .first.get();

                op->set_comm(msg->source(), m.tid(), m.seqno(), t);

                update_state(op->source(), op->tid(), op->seqno(),
                             transfer_state::running, -1.0f);
                // Different scenarios read -> write | write -> read

                cargo::error_code ec = (*op)();
                if(ec != cargo::error_code::transfer_in_progress) {
                    update_state(op->source(), op->tid(), op->seqno(),
                                 transfer_state::failed, -1.0f, ec);
                    m_ops.erase(make_pair(m.input_path(), m.output_path()));
                }
                break;
            }

            case tag::bw_shaping: {
                shaper_message m;
                world.recv(msg->source(), msg->tag(), m);
                LOGGER_INFO("msg => from: {} body: {}", msg->source(), m);
                for(auto I = m_ops.begin(); I != m_ops.end(); I++) {
                    const auto op = I->second.first.get();
                    if(op) {
                        op->set_bw_shaping(m.shaping());
                    } else {
                        LOGGER_INFO("Operation non existent", msg->source(), m);
                    }
                }


                break;
            }


            case tag::shutdown:
                LOGGER_INFO("msg => from: {} body: {{shutdown}}",
                            msg->source());
                world.recv(msg->source(), msg->tag());
                done = true;
                break;

            default:
                LOGGER_WARN("[{}] Unexpected message tag: {}", msg->source(),
                            msg->tag());
                break;
        }
    }

    LOGGER_INFO("Entering exit barrier...");
    world.barrier();
    LOGGER_INFO("Exit");

    return 0;
}

} // namespace cargo
