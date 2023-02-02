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

namespace {

std::string
get_address(auto&& req) {
    return req.get_endpoint();
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
