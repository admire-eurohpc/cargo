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

#ifndef RPC_SERVER_HPP
#define RPC_SERVER_HPP

#include <optional>
#include <logger/logger.hpp>
#include <utility>
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include "signal_listener.hpp"

namespace network {

class endpoint;

using request = thallium::request;

template <typename T>
using provider = thallium::provider<T>;

class server {

public:
    server(std::string name, std::string address, bool daemonize,
           std::filesystem::path rundir,
           std::optional<std::filesystem::path> pidfile = {});

    ~server();

    template <typename... Args>
    void
    configure_logger(logger::logger_type type, Args&&... args) {
        m_logger_config = logger::logger_config(m_name, type,
                                                std::forward<Args>(args)...);
    }

    std::optional<endpoint>
    lookup(const std::string& address) noexcept;

    std::string
    self_address() const noexcept;

    int
    run();
    void
    shutdown();
    void
    teardown();
    void
    teardown_and_exit();

    template <typename Callable>
    void
    set_handler(const std::string& name, Callable&& handler) {
        m_network_engine.define(name, handler);
    }

private:
    int
    daemonize();
    void
    signal_handler(int);
    void
    init_logger();
    void
    install_signal_handlers();
    void
    print_greeting();
    void
    print_farewell();

protected:
    virtual void
    check_configuration() const;

    virtual void
    print_configuration() const;

private:
    std::string m_name;
    std::string m_address;
    bool m_daemonize;
    std::filesystem::path m_rundir;
    std::optional<std::filesystem::path> m_pidfile;
    logger::logger_config m_logger_config;

protected:
    thallium::engine m_network_engine;

private:
    signal_listener m_signal_listener;
};


} // namespace network

#endif // RPC_SERVER_HPP
