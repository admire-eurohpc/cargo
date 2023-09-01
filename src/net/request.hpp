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

#ifndef NETWORK_REQUEST_HPP
#define NETWORK_REQUEST_HPP

#include <thallium.hpp>

namespace network {

using request = thallium::request;

template <typename Request>
inline std::string
get_address(Request&& req) {
    return std::forward<Request>(req).get_endpoint();
}

} // namespace network

#endif // NETWORK_REQUEST_HPP
