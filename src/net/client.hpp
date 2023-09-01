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

#ifndef NETWORK_CLIENT_HPP
#define NETWORK_CLIENT_HPP

#include <optional>
#include <thallium.hpp>

namespace network {

class endpoint;

class client {

public:
    explicit client(const std::string& protocol);
    std::optional<endpoint>
    lookup(const std::string& address) noexcept;
    std::string
    self_address() const noexcept;

private:
    thallium::engine m_engine;
};

} // namespace network

#endif // NETWORK_CLIENT_HPP
