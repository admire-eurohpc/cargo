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


#ifndef NETWORK_UTILITIES_HPP
#define NETWORK_UTILITIES_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <atomic>
#include <fmt/format.h>

namespace network {

class rpc_info {
private:
    static std::uint64_t
    new_id() {
        static std::atomic_uint64_t s_current_id;
        return s_current_id++;
    }

public:
    rpc_info(std::uint64_t id, std::string name, std::string address)
        : m_id(id), m_children(0), m_name(std::move(name)),
          m_address(std::move(address)) {}

    rpc_info(std::uint64_t id, std::uint64_t pid, std::string name,
             std::string address)
        : m_id(id), m_pid(pid), m_children(0), m_name(std::move(name)),
          m_address(std::move(address)) {}

    template <typename... Args>
    static rpc_info
    create(Args&&... args) {
        return {new_id(), std::forward<Args>(args)...};
    }

    template <typename... Args>
    rpc_info
    add_child(std::string address) const {
        return {m_children, m_id, m_name, std::move(address)};
    }

    constexpr std::uint64_t
    id() const {
        return m_id;
    }

    constexpr std::optional<std::uint64_t>
    pid() const {
        return m_pid;
    }

    const std::string&
    name() const {
        return m_name;
    }

    const std::string&
    address() const {
        return m_address;
    }

private:
    std::uint64_t m_id;
    std::optional<std::uint64_t> m_pid;
    std::uint64_t m_children;
    std::string m_name;
    std::string m_address;
};

} // namespace network

template <>
struct fmt::formatter<network::rpc_info> {

    // RPC direction format:
    //   '<': from self to target (outbound)
    //   '>': from target to self (inbound)
    bool m_outbound = true;

    // Parses format specifications in the form:
    constexpr auto
    parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();

        if(it != end && (*it == '<' || *it == '>')) {
            m_outbound = *it++ == '<';
        }

        if(it != end && *it != '}') {
            ctx.on_error("invalid format");
        }

        return it;
    }

    template <typename FormatContext>
    constexpr auto
    format(const network::rpc_info& rpc, FormatContext& ctx) const {
        format_to(ctx.out(), "{}{} id: {} name: {} ", m_outbound ? "<=" : "=>",
                  rpc.pid() ? fmt::format(" pid: {}", *rpc.pid()) : "",
                  rpc.id(), std::quoted(rpc.name()));
        return m_outbound ? format_to(ctx.out(), "to: {}",
                                      std::quoted(rpc.address()))
                          : format_to(ctx.out(), "from: {}",
                                      std::quoted(rpc.address()));
    }
};

#endif // NETWORK_UTILITIES_HPP
