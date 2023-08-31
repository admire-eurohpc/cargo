/******************************************************************************
 * Copyright 2023, Barcelona Supercomputing Center (BSC), Spain
 *
 * This software was partially supported by the EuroHPC-funded project ADMIRE
 *   (Project ID: 956748, https://www.admire-eurohpc.eu).
 *
 * This file is part of mochi-rpc-server-cxx.
 *
 * mochi-rpc-server-cxx is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mochi-rpc-server-cxx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mochi-rpc-server-cxx.  If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *****************************************************************************/

#include <optional>
#include <logger/logger.hpp>
#include "client.hpp"
#include "endpoint.hpp"

using namespace std::literals;

namespace network {


client::client(const std::string& protocol)
    : m_engine(protocol, THALLIUM_CLIENT_MODE) {}

std::optional<endpoint>
client::lookup(const std::string& address) noexcept {
    try {
        return endpoint{m_engine, m_engine.lookup(address)};
    } catch(const std::exception& ex) {
        LOGGER_ERROR("client::lookup() failed: {}", ex.what());
        return std::nullopt;
    }
}

std::string
client::self_address() const noexcept {
    try {
        return m_engine.self();
    } catch(const std::exception& ex) {
        LOGGER_ERROR("client::self_address() failed: {}", ex.what());
        return "unknown"s;
    }
}

} // namespace network
