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
#include "message.hpp"
#include "master.hpp"
#include "net/utilities.hpp"
#include "net/request.hpp"
#include "proto/rpc/response.hpp"

using namespace std::literals;
namespace mpi = boost::mpi;

namespace {

cargo::transfer_request
make_request(const cargo::dataset& input, const cargo::dataset& output) {

    static std::uint64_t id = 0;
    cargo::transfer_type tx_type;

    if(input.supports_parallel_transfer()) {
        tx_type = cargo::parallel_read;
    } else if(output.supports_parallel_transfer()) {
        tx_type = cargo::parallel_write;
    } else {
        tx_type = cargo::sequential;
    }

    return cargo::transfer_request{id++, input.path(), output.path(), tx_type};
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
                transfer_status st;
                world.recv(mpi::any_source, msg->tag(), st);
                LOGGER_INFO("[{}] Status received: {}", msg->source(), st);
                break;
            }

            default:
                LOGGER_WARN("[{}] Unexpected message tag: {}", msg->source(),
                            msg->tag());
                break;
        }
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

    const auto resp = generic_response{rpc.id(), error_code{0}};

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

    const auto rpc = rpc_info::create(RPC_NAME(), get_address(req));

    LOGGER_INFO("rpc {:>} body: {{sources: {}, targets: {}}}", rpc, sources,
                targets);

    const auto resp = generic_response{rpc.id(), error_code{0}};

    assert(sources.size() == targets.size());

    mpi::communicator world;
    for(auto i = 0u; i < sources.size(); ++i) {

        const auto& input_path = sources[i].path();
        const auto& output_path = targets[i].path();

        const auto m = ::make_request(sources[i], targets[i]);

        for(int rank = 1; rank < world.size(); ++rank) {
            world.send(rank, static_cast<int>(tag::transfer), m);
        }
    }

    transfer tx{42};

    LOGGER_INFO("rpc {:<} body: {{retval: {}, transfer: {}}}", rpc,
                resp.error_code(), tx);

    req.respond(resp);
}

} // namespace cargo
