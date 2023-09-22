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
        return std::make_tuple(static_cast<int>(cargo::tag::pread),
                               cargo::transfer_message{tid, seqno, input.path(),
                                                       output.path()});
    }

    if(output.supports_parallel_transfer()) {
        return std::make_tuple(static_cast<int>(cargo::tag::pwrite),
                               cargo::transfer_message{tid, seqno, input.path(),
                                                       output.path()});
    }

    return std::make_tuple(
            static_cast<int>(cargo::tag::sequential),
            cargo::transfer_message{tid, seqno, input.path(), output.path()});
}

} // namespace

using namespace std::literals;

namespace cargo {

master_server::master_server(std::string name, std::string address,
                             bool daemonize, std::filesystem::path rundir,
                             std::optional<std::filesystem::path> pidfile)
    : server(std::move(name), std::move(address), daemonize, std::move(rundir),
             std::move(pidfile)),
      provider(m_network_engine, 0),
      m_mpi_listener_ess(thallium::xstream::create()),
      m_mpi_listener_ult(m_mpi_listener_ess->make_thread(
              [this]() { mpi_listener_ult(); })) {

#define EXPAND(rpc_name) #rpc_name##s, &master_server::rpc_name
    provider::define(EXPAND(ping));
    provider::define(EXPAND(transfer_datasets));

#undef EXPAND

    // ESs and ULTs need to be joined before the network engine is
    // actually finalized, and ~master_server() is too late for that.
    // The push_prefinalize_callback() and push_finalize_callback() functions
    // serve this purpose. The former is called before Mercury is finalized,
    // while the latter is called in between that and Argobots finalization.
    m_network_engine.push_finalize_callback([this]() {
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
            thallium::thread::self().sleep(m_network_engine, 150);
            continue;
        }

        switch(static_cast<cargo::tag>(msg->tag())) {
            case tag::status: {
                status_message m;
                world.recv(mpi::any_source, msg->tag(), m);
                LOGGER_INFO("msg => from: {} body: {{status: {}}}",
                            msg->source(), m);

                m_request_manager.update(m.tid(), m.seqno(), msg->source() - 1,
                                         m.error_code()
                                                 ? part_status::failed
                                                 : part_status::completed);
                break;
            }

            default:
                LOGGER_WARN("msg => from: {} body: {{Unexpected tag: {}}}",
                            msg->source(), msg->tag());
                break;
        }
    }

    // shutting down, notify all workers
    for(int rank = 1; rank < world.size(); ++rank) {
        LOGGER_INFO("msg <= to: {} body: {{shutdown}}", rank);
        world.send(static_cast<int>(rank), static_cast<int>(tag::shutdown));
    }
}

#define RPC_NAME() ("ADM_"s + __FUNCTION__)

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

    m_request_manager.create(sources.size(), world.size() - 1)
            .or_else([&](auto&& ec) {
                LOGGER_ERROR("Failed to create request: {}", ec);
                LOGGER_INFO("rpc {:<} body: {{retval: {}}}", rpc, ec);
                req.respond(generic_response{rpc.id(), ec});
            })
            .map([&](auto&& r) {
                assert(sources.size() == targets.size());

                for(auto i = 0u; i < sources.size(); ++i) {
                    const auto& s = sources[i];
                    const auto& d = targets[i];

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

} // namespace cargo
