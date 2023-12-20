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
            static_cast<int>(cargo::tag::seq_mixed),
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
              [this]() { mpi_listener_ult(); })) {

#define EXPAND(rpc_name) #rpc_name##s, &master_server::rpc_name
    provider::define(EXPAND(ping));
    provider::define(EXPAND(shutdown));
    provider::define(EXPAND(transfer_datasets));
    provider::define(EXPAND(transfer_status));
    provider::define(EXPAND(bw_control));
    provider::define(EXPAND(transfer_statuses));

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
    });
}

master_server::~master_server() {}

void
master_server::mpi_listener_ult() {

    mpi::communicator world;
    while(!m_shutting_down) {

        auto msg = world.iprobe();

        if(!msg) {
            thallium::thread::self().sleep(m_network_engine, 10);
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


    // As we accept directories expanding directories should be done before and
    // update sources and targets.

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

        std::vector<std::filesystem::path> files;
        if(std::filesystem::is_directory(p)) {
            LOGGER_INFO("Expanding input directory {}", p);
            for(const auto& f :
                std::filesystem::recursive_directory_iterator(p)) {
                if(std::filesystem::is_regular_file(f)) {
                    files.push_back(f.path());
                }
            }

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

                d_new.path(d.path() / std::filesystem::path(
                                              f.string().substr(leading + 1)));

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

                // For all the files
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

                    for(std::size_t rank = 1; rank <= r.nworkers(); ++rank) {
                        const auto [t, m] = make_message(r.tid(), i, s, d);
                        LOGGER_INFO("msg <= to: {} body: {}", rank, m);
                        world.send(static_cast<int>(rank), t, m);
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

} // namespace cargo
