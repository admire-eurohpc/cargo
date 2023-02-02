/******************************************************************************
 * Copyright 2022-2023, Barcelona Supercomputing Center (BSC), Spain
 *
 * This software was partially supported by the EuroHPC-funded project ADMIRE
 *   (Project ID: 956748, https://www.admire-eurohpc.eu).
 *
 * This file is part of scord.
 *
 * scord is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * scord is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with scord.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *****************************************************************************/

#ifndef CONFIG_SETTINGS_HPP
#define CONFIG_SETTINGS_HPP

#include <cstdint>
#include <string>
#include <list>
#include <filesystem>
#include <utility>
#include <netinet/in.h>

#include "defaults.hpp"

namespace fs = std::filesystem;

namespace config {

struct namespace_def {

    namespace_def(std::string nsid, bool track, fs::path mountpoint,
                  std::string alias, const uint64_t capacity,
                  std::string visibility)
        : m_nsid(std::move(nsid)), m_track(track),
          m_mountpoint(std::move(mountpoint)), m_alias(std::move(alias)),
          m_capacity(capacity), m_visibility(std::move(visibility)) {}

    namespace_def(const namespace_def& other) = default;

    namespace_def(namespace_def&& rhs) = default;

    namespace_def&
    operator=(const namespace_def& other) = default;

    namespace_def&
    operator=(namespace_def&& rhs) = default;

    std::string
    nsid() const {
        return m_nsid;
    }

    bool
    track() const {
        return m_track;
    }

    fs::path
    mountpoint() const {
        return m_mountpoint;
    }

    std::string
    alias() const {
        return m_alias;
    }

    uint64_t
    capacity() const {
        return m_capacity;
    }

    std::string
    visibility() const {
        return m_visibility;
    }

    std::string m_nsid;
    bool m_track;
    fs::path m_mountpoint;
    std::string m_alias;
    uint64_t m_capacity;
    std::string m_visibility;
};

struct settings {

    settings();

    settings(std::string progname, bool daemonize, bool use_syslog,
             bool use_console, fs::path log_file,
             const uint32_t log_file_max_size, std::string transport_protocol,
             std::string bind_address, uint32_t remote_port, fs::path pidfile,
             uint32_t workers, uint32_t backlog_size, fs::path cfgfile,
             std::list<namespace_def> defns);

    void
    load_defaults();

    void
    load_from_file(const fs::path& filename);

    std::string
    to_string() const;

    settings(const settings& other) = default;

    settings(settings&& rhs) = default;

    settings&
    operator=(const settings& other) = default;

    settings&
    operator=(settings&& rhs) = default;

    ~settings() = default;

    std::string
    progname() const;

    void
    progname(const std::string& progname);

    bool
    daemonize() const;

    void
    daemonize(bool daemonize);

    bool
    use_syslog() const;

    void
    use_syslog(bool use_syslog);

    bool
    use_console() const;

    void
    use_console(bool use_console);

    fs::path
    log_file() const;

    void
    log_file(const fs::path& log_file);

    uint32_t
    log_file_max_size() const;

    void
    log_file_max_size(uint32_t log_file_max_size);

    std::string
    transport_protocol() const;

    void
    transport_protocol(const std::string& transport_protocol);

    std::string
    bind_address() const;

    void
    bind_address(const std::string& bind_address);

    in_port_t
    remote_port() const;

    void
    remote_port(in_port_t remote_port);

    fs::path
    pidfile() const;

    void
    pidfile(const fs::path& pidfile);

    uint32_t
    workers_in_pool() const;

    void
    workers_in_pool(uint32_t workers_in_pool);

    uint32_t
    backlog_size() const;

    void
    backlog_size(uint32_t backlog_size);

    fs::path
    config_file() const;

    void
    config_file(const fs::path& config_file);

    std::list<namespace_def>
    default_namespaces() const;

    void
    default_namespaces(const std::list<namespace_def>& default_namespaces);

    std::string m_progname = defaults::progname;
    bool m_daemonize = defaults::daemonize;
    bool m_use_syslog = defaults::use_syslog;
    bool m_use_console = defaults::use_console;
    fs::path m_log_file = defaults::log_file;
    uint32_t m_log_file_max_size = defaults::log_file_max_size;
    std::string m_transport_protocol = defaults::transport_protocol;
    std::string m_bind_address = defaults::bind_address;
    in_port_t m_remote_port = defaults::remote_port;
    fs::path m_daemon_pidfile = defaults::pidfile;
    uint32_t m_workers_in_pool = defaults::workers_in_pool;
    uint32_t m_backlog_size = defaults::backlog_size;
    fs::path m_config_file = defaults::config_file;
    std::list<namespace_def> m_default_namespaces;
};

} // namespace config

#endif /* CONFIG_SETTINGS_HPP */
