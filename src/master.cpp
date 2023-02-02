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
#include <config/settings.hpp>
#include <logger/logger.hpp>
#include <net/server.hpp>

#include <cargo.hpp>
#include <fmt_formatters.hpp>
#include <boost/mpi.hpp>
#include "message.hpp"

using namespace std::literals;

namespace {

std::string
get_address(auto&& req) {
    return req.get_endpoint();
}

std::tuple<std::string, std::string>
split(const std::string& id) {

    constexpr auto delim = "://"sv;
    const auto n = id.find(delim);

    if(n == std::string::npos) {
        return {std::string{}, id};
    }

    return {id.substr(0, n), id.substr(n + delim.length(), id.length())};
}

cargo::transfer_request_message
create_request_message(const cargo::dataset& input,
                       const cargo::dataset& output) {

    cargo::transfer_type tx_type;

    const auto& [input_prefix, input_path] = split(input.id());
    const auto& [output_prefix, output_path] = split(output.id());

    // FIXME: id should offer member functions to retrieve the parent
    // namespace
    if(input_prefix == "lustre") {
        tx_type = cargo::parallel_read;
    } else if(output_prefix == "lustre") {
        tx_type = cargo::parallel_write;
    } else {
        tx_type = cargo::sequential;
    }

    return cargo::transfer_request_message{input_path, output_path, tx_type};
}

} // namespace

using namespace std::literals;

struct remote_procedure {
    static std::uint64_t
    new_id() {
        static std::atomic_uint64_t current_id;
        return current_id++;
    }
};

namespace handlers {

void
ping(const thallium::request& req) {

    const auto rpc_id = remote_procedure::new_id();

    LOGGER_INFO("rpc id: {} name: {} from: {} => "
                "body: {{}}",
                rpc_id, std::quoted(__FUNCTION__),
                std::quoted(get_address(req)));

    const auto retval = cargo::error_code{0};

    LOGGER_INFO("rpc id: {} name: {} to: {} <= "
                "body: {{retval: {}}}",
                rpc_id, std::quoted(__FUNCTION__),
                std::quoted(get_address(req)), retval);

    req.respond(retval);
}

void
transfer_datasets(const net::request& req,
                  const std::vector<cargo::dataset>& sources,
                  const std::vector<cargo::dataset>& targets) {

    const auto rpc_id = remote_procedure::new_id();
    LOGGER_INFO("rpc id: {} name: {} from: {} => "
                "body: {{sources: {}, targets: {}}}",
                rpc_id, std::quoted(__FUNCTION__),
                std::quoted(get_address(req)), sources, targets);

    const auto retval = cargo::error_code{0};

    assert(sources.size() == targets.size());

    boost::mpi::communicator world;
    for(auto i = 0u; i < sources.size(); ++i) {

        const auto& input_path = sources[i].id();
        const auto& output_path = targets[i].id();

        const auto m = ::create_request_message(sources[i], targets[i]);

        for(int rank = 1; rank < world.size(); ++rank) {
            world.send(rank, static_cast<int>(cargo::message_tags::transfer),
                       m);
        }
    }

    cargo::transfer tx{42};

    LOGGER_INFO("rpc id: {} name: {} to: {} <= "
                "body: {{retval: {}, transfer: {}}}",
                rpc_id, std::quoted(__FUNCTION__),
                std::quoted(get_address(req)), retval, tx);

    req.respond(retval);
}


} // namespace handlers

void
master(const config::settings& cfg) {

    net::server daemon(cfg);

    daemon.set_handler("ping"s, handlers::ping);
    daemon.set_handler("transfer_datasets"s, handlers::transfer_datasets);
    daemon.run();
}
