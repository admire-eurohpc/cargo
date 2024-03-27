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

#include <functional>
#include <logger/logger.hpp>
#include <net/server.hpp>

#include <cargo.hpp>
#include <fmt_formatters.hpp>
#include <boost/mpi.hpp>
#include <utility>
#include <boost/mpi/communicator.hpp>
#include "master.hpp"
#include "net/utilities.hpp"
#include "net/request.hpp"
#include "proto/rpc/response.hpp"
#include "proto/mpi/message.hpp"
#include "parallel_request.hpp"

using namespace std::literals;
namespace mpi = boost::mpi;

namespace {

std::tuple<int, cargo::transfer_message>
make_message(std::uint64_t tid, std::uint32_t seqno,
             const cargo::dataset& input, const cargo::dataset& output) {

    if(input.supports_parallel_transfer()) {
        return std::make_tuple(
                static_cast<int>(cargo::tag::pread),
                cargo::transfer_message{
                        tid, seqno, input.path(),
                        static_cast<uint32_t>(input.get_type()), output.path(),
                        static_cast<uint32_t>(output.get_type())});
    }

    if(output.supports_parallel_transfer()) {
        return std::make_tuple(
                static_cast<int>(cargo::tag::pwrite),
                cargo::transfer_message{
                        tid, seqno, input.path(),
                        static_cast<uint32_t>(input.get_type()), output.path(),
                        static_cast<uint32_t>(output.get_type())});
    }

    return std::make_tuple(
            static_cast<int>(cargo::tag::sequential),
            cargo::transfer_message{tid, seqno, input.path(),
                                    static_cast<uint32_t>(input.get_type()),
                                    output.path(),
                                    static_cast<uint32_t>(input.get_type())});
}

} // namespace

using namespace std::literals;

namespace cargo {

master_server::master_server(std::string name, std::string address,
                             bool daemonize, std::filesystem::path rundir,
                             std::uint64_t block_size,
                             std::optional<std::filesystem::path> pidfile)
    : server(std::move(name), std::move(address), daemonize, std::move(rundir),
             std::move(block_size), std::move(pidfile)),
      provider(m_network_engine, 0),
      m_mpi_listener_ess(thallium::xstream::create()),
      m_mpi_listener_ult(m_mpi_listener_ess->make_thread(
              [this]() { mpi_listener_ult(); })),
      m_ftio_listener_ess(thallium::xstream::create()),
      m_ftio_listener_ult(m_ftio_listener_ess->make_thread(
              [this]() { ftio_scheduling_ult(); }))

{

#define EXPAND(rpc_name) #rpc_name##s, &master_server::rpc_name
    provider::define(EXPAND(ping));
    provider::define(EXPAND(shutdown));
    provider::define(EXPAND(transfer_datasets));
    provider::define(EXPAND(transfer_status));
    provider::define(EXPAND(bw_control));
    provider::define(EXPAND(transfer_statuses));
    provider::define(EXPAND(ftio_int));

#undef EXPAND

    // ESs and ULTs need to be joined before the network engine is
    // actually finalized, and ~master_server() is too late for that.
    // The push_prefinalize_callback() and push_finalize_callback() functions
    // serve this purpose. The former is called before Mercury is finalized,
    // while the latter is called in between that and Argobots finalization.
    m_network_engine.push_prefinalize_callback([this]() {
        m_mpi_listener_ult->join();
        m_mpi_listener_ult = thallium::managed<thallium::thread>{};
        m_mpi_listener_ess->join();
        m_mpi_listener_ess = thallium::managed<thallium::xstream>{};
        m_ftio_listener_ult->join();
        m_ftio_listener_ult = thallium::managed<thallium::thread>{};
        m_ftio_listener_ess->join();
        m_ftio_listener_ess = thallium::managed<thallium::xstream>{};
    });
}

master_server::~master_server() {}

void
master_server::mpi_listener_ult() {

    mpi::communicator world;
    while(!m_shutting_down) {

        auto msg = world.iprobe();

        if(!msg) {
            std::this_thread::sleep_for(10ms);
            // thallium::thread::self().sleep(m_network_engine, 10);
            continue;
        }

        switch(static_cast<cargo::tag>(msg->tag())) {
            case tag::status: {
                status_message m;
                world.recv(msg->source(), msg->tag(), m);
                LOGGER_DEBUG("msg => from: {} body: {{payload: {}}}",
                             msg->source(), m);

                m_request_manager.update(m.tid(), m.seqno(), msg->source() - 1,
                                         m.name(), m.state(), m.bw(),
                                         m.error_code());
                break;
            }

            default:
                LOGGER_WARN("msg => from: {} body: {{Unexpected tag: {}}}",
                            msg->source(), msg->tag());
                break;
        }
    }

    LOGGER_INFO("Shutting down. Notifying workers...");

    // shutting down, notify all workers
    for(int rank = 1; rank < world.size(); ++rank) {
        LOGGER_INFO("msg <= to: {} body: {{shutdown}}", rank);
        world.send(static_cast<int>(rank), static_cast<int>(tag::shutdown));
    }

    LOGGER_INFO("Entering exit barrier...");
    world.barrier();

    LOGGER_INFO("Exit");
}


void
master_server::ftio_scheduling_ult() {

    while(!m_shutting_down) {

        if(!m_pending_transfer.m_work or !m_ftio_run) {
            std::this_thread::sleep_for(1000ms);
        }
        //        if(!m_pending_transfer.m_work or m_period < 0.0f) {
        //            std::this_thread::sleep_for(1000ms);
        //        }


        // Do something with the confidence and probability

        //        if(m_ftio_run) {
        //            m_ftio_run = false;
        //            LOGGER_INFO("Confidence is {}, probability is {} and
        //            period is {}",
        //                        m_confidence, m_probability, m_period);
        //        }

        if(!m_pending_transfer.m_work)
            continue;
        if(m_period > 0) {
            LOGGER_INFO("Waiting period : {}", m_period);
        } else {
            LOGGER_INFO("Waiting for run trigger ...");
        }
        // Wait in small periods, just in case we change it, This should be
        // mutexed...
        auto elapsed = m_period;
        while(elapsed > 0) {
            std::this_thread::sleep_for(std::chrono::seconds((int) (1)));
            elapsed -= 1;
            // reset elapsed value when new RPC comes in
            if(m_ftio_run) {
                elapsed = m_period;
                m_ftio_run = false;
            }
        }
        if(!m_ftio_run) {
            continue;
        }

        LOGGER_INFO("Checking if there is work to do in {}",
                    m_pending_transfer.m_sources);
        transfer_dataset_internal(m_pending_transfer);
        // This launches the workers to do the work...
        // We wait until this transfer is finished
        LOGGER_INFO("Transferring : {}", m_pending_transfer.m_expanded_sources);
        bool finished = false;
        while(!finished) {
            std::this_thread::sleep_for(10ms);
            m_request_manager.lookup(m_pending_transfer.m_p.tid())
                    .or_else([&](auto&& ec) {
                        LOGGER_ERROR("Failed to lookup request: {}", ec);
                    })
                    .map([&](auto&& rs) {
                        if(rs.state() == transfer_state::completed) {
                            finished = true;
                        }
                    });
        }

        if(finished) {
            // Delete all source files
            LOGGER_INFO("Transfer finished for {}",
                        m_pending_transfer.m_expanded_sources);
            auto fs = FSPlugin::make_fs(cargo::FSPlugin::type::gekkofs);
            for(auto& file : m_pending_transfer.m_expanded_sources) {
                LOGGER_INFO("Deleting {}", file.path());
                // We need to use gekkofs to delete
                fs->unlink(file.path());
            }
        }
        if(m_period > 0) {
            // always run whenever period is set
            m_ftio_run = true;
        } else {
            m_ftio_run = false;
        }
    }

    LOGGER_INFO("Shutting down.");
}

#define RPC_NAME() (__FUNCTION__)

void
master_server::ping(const network::request& req) {
    using network::get_address;
    using network::rpc_info;
    using proto::generic_response;

    const auto rpc = rpc_info::create(RPC_NAME(), get_address(req));

    LOGGER_INFO("rpc {:>} body: {{}}", rpc);

    const auto resp = generic_response{rpc.id(), error_code::success};

    LOGGER_INFO("rpc {:<} body: {{retval: {}}}", rpc, resp.error_code());

    req.respond(resp);
}


void
master_server::bw_control(const network::request& req, std::uint64_t tid,
                          std::int16_t shaping) {
    using network::get_address;
    using network::rpc_info;
    using proto::generic_response;
    mpi::communicator world;
    const auto rpc = rpc_info::create(RPC_NAME(), get_address(req));

    LOGGER_INFO("rpc {:>} body: {{tid: {}, shaping: {}}}", rpc, tid, shaping);

    for(int rank = 1; rank < world.size(); ++rank) {
        const auto m = cargo::shaper_message{tid, shaping};
        LOGGER_INFO("msg <= to: {} body: {}", rank, m);
        world.send(static_cast<int>(rank), static_cast<int>(tag::bw_shaping),
                   m);
    }

    const auto resp = generic_response{rpc.id(), error_code::success};

    LOGGER_INFO("rpc {:<} body: {{retval: {}}}", rpc, resp.error_code());

    req.respond(resp);
}

void
master_server::shutdown(const network::request& req) {
    using network::get_address;
    using network::rpc_info;
    using proto::generic_response;

    const auto rpc = rpc_info::create(RPC_NAME(), get_address(req));

    LOGGER_INFO("rpc {:>} body: {{}}", rpc);
    server::shutdown();
}

// Function that gets a pending_request, fills the request and sends the mpi
// message for the transfer We only put files that has mtime < actual
// timestamp , intended for stage-out and ftio
void
master_server::transfer_dataset_internal(pending_transfer& pt) {

    mpi::communicator world;
    std::vector<cargo::dataset> v_s_new;
    std::vector<cargo::dataset> v_d_new;
    time_t now = time(0);
    now = now - 5; // Threshold for mtime
    for(auto i = 0u; i < pt.m_sources.size(); ++i) {

        const auto& s = pt.m_sources[i];
        const auto& d = pt.m_targets[i];

        // We need to expand directories to single files on the s
        // Then create a new message for each file and append the
        // file to the d prefix
        // We will asume that the path is the original absolute
        // The prefix selects the method of transfer
        // And if not specified then we will use none
        // i.e. ("xxxx:/xyyy/bbb -> gekko:/cccc/ttt ) then
        // bbb/xxx -> ttt/xxx
        const auto& p = s.path();

        std::vector<std::string> files;
        // Check stat of p using FSPlugin class
        auto fs = FSPlugin::make_fs(
                static_cast<cargo::FSPlugin::type>(s.get_type()));
        struct stat buf;
        fs->stat(p, &buf);
        if(buf.st_mode & S_IFDIR) {
            LOGGER_INFO("Expanding input directory {}", p);
            files = fs->readdir(p);

            /*
            We have all the files expanded. Now create a new
            cargo::dataset for each file as s and a new
            cargo::dataset appending the base directory in d to the
            file name.
            */
            for(const auto& f : files) {
                cargo::dataset s_new(s);
                cargo::dataset d_new(d);
                s_new.path(f);
                // We need to get filename from the original root
                // path (d.path) plus the path from f, removing the
                // initial path p (taking care of the trailing /)
                auto leading = p.size();
                if(leading > 0 and p.back() == '/') {
                    leading--;
                }

                d_new.path(d.path() /
                           std::filesystem::path(f.substr(leading + 1)));

                LOGGER_DEBUG("Expanded file {} -> {}", s_new.path(),
                             d_new.path());
                fs->stat(s_new.path(), &buf);
                if(buf.st_mtime < now) {
                    v_s_new.push_back(s_new);
                    v_d_new.push_back(d_new);
                }
            }
        } else {
            fs->stat(s.path(), &buf);
            if(buf.st_mtime < now) {
                v_s_new.push_back(s);
                v_d_new.push_back(d);
            }
        }
    }

    // empty m_expanded_sources
    pt.m_expanded_sources.assign(v_s_new.begin(), v_s_new.end());
    pt.m_expanded_targets.assign(v_d_new.begin(), v_d_new.end());

    // We have two vectors, so we process the transfer
    // [1] Update request_manager
    // [2] Send message to worker

    auto ec = m_request_manager.update(pt.m_p.tid(), v_s_new.size(),
                                       pt.m_p.nworkers());
    if(ec != error_code::success) {
        LOGGER_ERROR("Failed to update request: {}", ec);
        return;
    };

    assert(v_s_new.size() == v_d_new.size());

    // For all the transfers
    for(std::size_t i = 0; i < v_s_new.size(); ++i) {
        const auto& s = v_s_new[i];
        const auto& d = v_d_new[i];

        // Create the directory if it does not exist (only in
        // parallel transfer)
        if(!std::filesystem::path(d.path()).parent_path().empty() and
           d.supports_parallel_transfer()) {
            std::filesystem::create_directories(
                    std::filesystem::path(d.path()).parent_path());
        }


        // Send message to worker
        for(std::size_t rank = 1; rank <= pt.m_p.nworkers(); ++rank) {
            const auto [t, m] = make_message(pt.m_p.tid(), i, s, d);
            LOGGER_INFO("msg <= to: {} body: {}", rank, m);
            world.send(static_cast<int>(rank), t, m);
        }
    }
}

void
master_server::transfer_datasets(const network::request& req,
                                 const std::vector<dataset>& sources,
                                 const std::vector<dataset>& targets) {
    using network::get_address;
    using network::rpc_info;
    using proto::generic_response;
    using proto::response_with_id;

    mpi::communicator world;
    const auto rpc = rpc_info::create(RPC_NAME(), get_address(req));

    LOGGER_INFO("rpc {:>} body: {{sources: {}, targets: {}}}", rpc, sources,
                targets);


    // As we accept directories expanding directories should be done before
    // and update sources and targets.

    std::vector<cargo::dataset> v_s_new;
    std::vector<cargo::dataset> v_d_new;

    for(auto i = 0u; i < sources.size(); ++i) {

        const auto& s = sources[i];
        const auto& d = targets[i];


        // We need to expand directories to single files on the s
        // Then create a new message for each file and append the
        // file to the d prefix
        // We will asume that the path is the original absolute
        // The prefix selects the method of transfer
        // And if not specified then we will use none
        // i.e. ("xxxx:/xyyy/bbb -> gekko:/cccc/ttt ) then
        // bbb/xxx -> ttt/xxx
        const auto& p = s.path();

        std::vector<std::string> files;
        // Check stat of p using FSPlugin class
        auto fs = FSPlugin::make_fs(
                static_cast<cargo::FSPlugin::type>(s.get_type()));
        struct stat buf;
        fs->stat(p, &buf);
        if(buf.st_mode & S_IFDIR) {
            LOGGER_INFO("Expanding input directory {}", p);
            files = fs->readdir(p);


            /*
            We have all the files expanded. Now create a new
            cargo::dataset for each file as s and a new
            cargo::dataset appending the base directory in d to the
            file name.
            */
            for(const auto& f : files) {
                cargo::dataset s_new(s);
                cargo::dataset d_new(d);
                s_new.path(f);
                // We need to get filename from the original root
                // path (d.path) plus the path from f, removing the
                // initial path p (taking care of the trailing /)
                auto leading = p.size();
                if(leading > 0 and p.back() == '/') {
                    leading--;
                }

                d_new.path(d.path() /
                           std::filesystem::path(f.substr(leading + 1)));

                LOGGER_DEBUG("Expanded file {} -> {}", s_new.path(),
                             d_new.path());
                v_s_new.push_back(s_new);
                v_d_new.push_back(d_new);
            }

        } else {
            v_s_new.push_back(s);
            v_d_new.push_back(d);
        }
    }

    m_request_manager.create(v_s_new.size(), world.size() - 1)
            .or_else([&](auto&& ec) {
                LOGGER_ERROR("Failed to create request: {}", ec);
                LOGGER_INFO("rpc {:<} body: {{retval: {}}}", rpc, ec);
                req.respond(generic_response{rpc.id(), ec});
            })
            .map([&](auto&& r) {
                assert(v_s_new.size() == v_d_new.size());
                if(m_ftio) {
                    if(sources[0].get_type() == cargo::dataset::type::gekkofs) {

                        // We have only one pendingTransfer for FTIO
                        // that can be updated, the issue is that we
                        // need the tid.
                        m_pending_transfer.m_p = r;
                        m_pending_transfer.m_sources = sources;
                        m_pending_transfer.m_targets = targets;
                        m_pending_transfer.m_work = true;
                        LOGGER_INFO("Stored stage-out information");
                    }
                }
                // For all the transfers
                for(std::size_t i = 0; i < v_s_new.size(); ++i) {
                    const auto& s = v_s_new[i];
                    const auto& d = v_d_new[i];

                    // Create the directory if it does not exist (only in
                    // parallel transfer)
                    if(!std::filesystem::path(d.path())
                                .parent_path()
                                .empty() and
                       d.supports_parallel_transfer()) {
                        std::filesystem::create_directories(
                                std::filesystem::path(d.path()).parent_path());
                    }

                    // If we are not using ftio start transfer if we are on
                    // stage-out
                    if(!m_ftio) {
                        // If we are on stage-out


                        for(std::size_t rank = 1; rank <= r.nworkers();
                            ++rank) {
                            const auto [t, m] = make_message(r.tid(), i, s, d);
                            LOGGER_INFO("msg <= to: {} body: {}", rank, m);
                            world.send(static_cast<int>(rank), t, m);
                        }
                    }
                }
                LOGGER_INFO("rpc {:<} body: {{retval: {}, tid: {}}}", rpc,
                            error_code::success, r.tid());
                req.respond(response_with_id{rpc.id(), error_code::success,
                                             r.tid()});
            });
}

void
master_server::transfer_status(const network::request& req, std::uint64_t tid) {

    using network::get_address;
    using network::rpc_info;
    using proto::generic_response;
    using proto::status_response;

    using response_type =
            status_response<cargo::transfer_state, float, cargo::error_code>;

    mpi::communicator world;
    const auto rpc = rpc_info::create(RPC_NAME(), get_address(req));

    LOGGER_INFO("rpc {:>} body: {{tid: {}}}", rpc, tid);

    m_request_manager.lookup(tid)
            .or_else([&](auto&& ec) {
                LOGGER_ERROR("Failed to lookup request: {}", ec);
                LOGGER_INFO("rpc {:<} body: {{retval: {}}}", rpc, ec);
                req.respond(generic_response{rpc.id(), ec});
            })
            .map([&](auto&& rs) {
                LOGGER_INFO("rpc {:<} body: {{retval: {}, status: {}}}", rpc,
                            error_code::success, rs);
                req.respond(response_type{
                        rpc.id(), error_code::success,
                        std::make_tuple(rs.state(), rs.bw(), rs.error())});
            });
}


void
master_server::transfer_statuses(const network::request& req,
                                 std::uint64_t tid) {

    using network::get_address;
    using network::rpc_info;
    using proto::generic_response;
    using proto::statuses_response;

    using response_type = statuses_response<std::string, cargo::transfer_state,
                                            float, cargo::error_code>;

    mpi::communicator world;
    const auto rpc = rpc_info::create(RPC_NAME(), get_address(req));

    LOGGER_INFO("rpc {:>} body: {{tid: {}}}", rpc, tid);
    // Get all the statuses of the associated transfer. returns a vector
    // of transfer_status objects

    m_request_manager.lookup_all(tid)
            .or_else([&](auto&& ec) {
                LOGGER_ERROR("Failed to lookup request: {}", ec);
                LOGGER_INFO("rpc {:<} body: {{retval: {}}}", rpc, ec);
                req.respond(generic_response{rpc.id(), ec});
            })
            .map([&](auto&& rs) {
                // We get a vector of request_status objects, we need to
                // convert them to a vector of tuples with the same
                // informations
                std::vector<std::tuple<std::string, cargo::transfer_state,
                                       float, std::optional<cargo::error_code>>>
                        v{};
                for(auto& r : rs) {
                    v.push_back(std::make_tuple(r.name(), r.state(), r.bw(),
                                                r.error()));
                    LOGGER_INFO(
                            "rpc {:<} body: {{retval: {}, name: {}, status: {}}}",
                            rpc, error_code::success, r.name(), r.state());
                }
                // Generate a response type with the vector of tuples and
                // respond


                req.respond(response_type{rpc.id(), error_code::success, v});
            });
}


void
master_server::ftio_int(const network::request& req, float conf, float prob,
                        float period, bool run) {
    using network::get_address;
    using network::rpc_info;
    using proto::generic_response;
    mpi::communicator world;
    const auto rpc = rpc_info::create(RPC_NAME(), get_address(req));

    m_confidence = conf;
    m_probability = prob;
    m_period = period;
    m_ftio_run = run;
    if(m_period > 0)
        m_ftio_run = true;
    m_ftio = true;

    LOGGER_INFO(
            "rpc {:>} body: {{confidence: {}, probability: {}, period: {}, run: {}}}",
            rpc, conf, prob, period, run);

    const auto resp = generic_response{rpc.id(), error_code::success};

    LOGGER_INFO("rpc {:<} body: {{retval: {}}}", rpc, resp.error_code());

    req.respond(resp);
}

} // namespace cargo
