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
#include <net/serialization.hpp>
#include <iomanip>
#include <logger/logger.hpp>
#include <net/client.hpp>
#include <net/utilities.hpp>
#include <net/endpoint.hpp>
#include <proto/rpc/response.hpp>
#include <utility>
#include <thread>

using namespace std::literals;

namespace cargo {

using generic_response = proto::generic_response<error_code>;
template <typename Value>
using response_with_value = proto::response_with_value<Value, error_code>;
using response_with_id = proto::response_with_id<error_code>;

#define RPC_NAME() (__FUNCTION__)

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

void
dataset::path(std::string path) {
    m_path = std::move(path);
};

bool
dataset::supports_parallel_transfer() const noexcept {
    return m_type == dataset::type::parallel;
}

transfer::transfer(transfer_id id, server srv) noexcept
    : m_id(id), m_srv(std::move(srv)) {}

[[nodiscard]] transfer_id
transfer::id() const noexcept {
    return m_id;
}

transfer_status
transfer::status() const {

    using proto::status_response;

    network::client rpc_client{m_srv.protocol()};
    const auto rpc =
            network::rpc_info::create("transfer_status", m_srv.address());
    using response_type = status_response<transfer_state, float, error_code>;

    if(const auto lookup_rv = rpc_client.lookup(m_srv.address());
       lookup_rv.has_value()) {
        const auto& endp = lookup_rv.value();

        LOGGER_INFO("rpc {:<} body: {{tid: {}}}", rpc, m_id);

        if(const auto call_rv = endp.call(rpc.name(), m_id);
           call_rv.has_value()) {

            const response_type resp{call_rv.value()};
            const auto& [s, bw, ec] = resp.value();

            LOGGER_EVAL(resp.error_code(), INFO, ERROR,
                        "rpc {:>} body: {{retval: {}}} [op_id: {}]", rpc,
                        resp.error_code(), resp.op_id());

            if(resp.error_code()) {
                throw std::runtime_error(
                        fmt::format("rpc call failed: {}", resp.error_code()));
            }

            return transfer_status{s, bw, ec.value_or(error_code::success)};
        }
    }

    throw std::runtime_error("rpc lookup failed");
}

transfer_status::transfer_status(transfer_state status, float bw,
                                 error_code error) noexcept
    : m_state(status), m_bw(bw), m_error(error) {}

transfer_state
transfer_status::state() const noexcept {
    return m_state;
}

bool
transfer_status::done() const noexcept {
    return m_state == transfer_state::completed;
}

bool
transfer_status::failed() const noexcept {
    return m_state == transfer_state::failed;
}

float
transfer_status::bw() const {
    return m_bw;
}

error_code
transfer_status::error() const {
    switch(m_state) {
        case transfer_state::pending:
            [[fallthrough]];
        case transfer_state::running:
            return error_code::transfer_in_progress;
        default:
            return m_error;
    }
}

transfer
transfer_datasets(const server& srv, const std::vector<dataset>& sources,
                  const std::vector<dataset>& targets) {

    if(sources.size() != targets.size()) {
        throw std::runtime_error(
                "The number of input datasets does not match the number of "
                "output datasets");
    }

    network::client rpc_client{srv.protocol()};
    const auto rpc = network::rpc_info::create(RPC_NAME(), srv.address());

    if(const auto lookup_rv = rpc_client.lookup(srv.address());
       lookup_rv.has_value()) {
        const auto& endp = lookup_rv.value();

        LOGGER_INFO("rpc {:<} body: {{sources: {}, targets: {}}}", rpc, sources,
                    targets);

        if(const auto call_rv = endp.call(rpc.name(), sources, targets);
           call_rv.has_value()) {

            const response_with_id resp{call_rv.value()};

            LOGGER_EVAL(resp.error_code(), INFO, ERROR,
                        "rpc {:>} body: {{retval: {}}} [op_id: {}]", rpc,
                        resp.error_code(), resp.op_id());

            if(resp.error_code()) {
                throw std::runtime_error(
                        fmt::format("rpc call failed: {}", resp.error_code()));
            }

            return transfer{resp.value(), srv};
        }
    }

    throw std::runtime_error("rpc lookup failed");
}

transfer_status
transfer::wait() const {
    // wait for the transfer to complete
    auto s = status();

    while(!s.done() && !s.failed()) {
        s = wait_for(150ms);
    }

    return s;
}

transfer_status
transfer::wait_for(const std::chrono::nanoseconds& timeout) const {
    std::this_thread::sleep_for(timeout);
    return status();
}

transfer
transfer_dataset(const server& srv, const dataset& source,
                 const dataset& target) {
    return transfer_datasets(srv, std::vector<dataset>{source},
                             std::vector<dataset>{target});
}

} // namespace cargo