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

#include <cargo.hpp>
#include <fmt_formatters.hpp>
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <iomanip>
#include <logger/logger.hpp>

using namespace std::literals;

namespace cargo {

struct remote_procedure {
    static std::uint64_t
    new_id() {
        static std::atomic_uint64_t current_id;
        return current_id++;
    }
};

server::server(std::string address) noexcept : m_address(std::move(address)) {

    const auto pos = m_address.rfind("://");

    if(pos != std::string::npos) {
        m_protocol = m_address.substr(0, pos);
    }
}

std::string
server::protocol() const noexcept {
    return m_protocol;
}

std::string
server::address() const noexcept {
    return m_address;
}

dataset::dataset(std::string path, dataset::type type) noexcept
    : m_path(std::move(path)), m_type(type) {}

std::string
dataset::path() const noexcept {
    return m_path;
};

bool
dataset::supports_parallel_transfer() const noexcept {
    return m_type == dataset::type::parallel;
}

transfer::transfer(transfer_id id) noexcept : m_id(id) {}

[[nodiscard]] transfer_id
transfer::id() const noexcept {
    return m_id;
}

cargo::transfer
transfer_datasets(const server& srv, const std::vector<dataset>& sources,
                  const std::vector<dataset>& targets) {

    thallium::engine engine(srv.protocol(), THALLIUM_CLIENT_MODE);
    thallium::remote_procedure transfer_datasets =
            engine.define("transfer_datasets");
    thallium::endpoint endp = engine.lookup(srv.address());

    const auto rpc_id = remote_procedure::new_id();

    LOGGER_INFO("rpc id: {} name: {} from: {} => "
                "body: {{sources: {}, targets: {}}}",
                rpc_id, std::quoted(__FUNCTION__),
                std::quoted(std::string{engine.self()}), sources, targets);

    cargo::error_code ec = transfer_datasets.on(endp)(sources, targets);
    const auto tx = cargo::transfer{42};
    const auto op_id = 42;

    LOGGER_INFO("rpc id: {} name: {} from: {} <= "
                "body: {{retval: {}, transfer: {}}} [op_id: {}]",
                rpc_id, std::quoted(__FUNCTION__),
                std::quoted(std::string{endp}), ec, tx, op_id);

    return tx;
}

} // namespace cargo