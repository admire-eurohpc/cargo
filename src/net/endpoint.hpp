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

#ifndef NETWORK_ENDPOINT_HPP
#define NETWORK_ENDPOINT_HPP

#include <thallium.hpp>
#include <optional>
#include <logger/logger.hpp>

namespace network {

enum class rpc_return_policy { requires_response, no_response };

class endpoint {

public:
    endpoint(thallium::engine& engine, thallium::endpoint endpoint);

    std::string
    address() const;

    template <rpc_return_policy rv = rpc_return_policy::requires_response,
              typename... Args>
    [[nodiscard]] std::optional<thallium::packed_data<>>
    call(const std::string& rpc_name, Args&&... args) const noexcept
            requires(rv == rpc_return_policy::requires_response) {

        try {
            const auto rpc = m_engine.define(rpc_name);
            return std::make_optional(
                    rpc.on(m_endpoint)(std::forward<Args>(args)...));
        } catch(const std::exception& ex) {
            LOGGER_ERROR("endpoint::call() failed: {}", ex.what());
            return std::nullopt;
        }
    }

    template <rpc_return_policy rv, typename... Args>
    void
    call(const std::string& rpc_name, Args&&... args) const noexcept
            requires(rv == rpc_return_policy::no_response) {

        try {
            const auto rpc = m_engine.define(rpc_name).disable_response();
            rpc.on(m_endpoint)(std::forward<Args>(args)...);
        } catch(const std::exception& ex) {
            LOGGER_ERROR("endpoint::call() failed: {}", ex.what());
        }
    }

    auto
    endp() const {
        return m_endpoint;
    }

    auto
    engine() const {
        return m_engine;
    }

private:
    mutable thallium::engine m_engine;
    thallium::endpoint m_endpoint;
};

} // namespace network

#endif // NETWORK_ENDPOINT_HPP
