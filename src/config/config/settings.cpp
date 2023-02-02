/******************************************************************************
 * Copyright 2021, Barcelona Supercomputing Center (BSC), Spain
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

#include <filesystem>
#include <utility>
#include "config_options.hpp"
#include "defaults.hpp"
#include "file_options/options_description.hpp"
#include "file_options/yaml_parser.hpp"
#include "keywords.hpp"
#include "settings.hpp"

namespace fs = std::filesystem;

namespace scord::config {

settings::settings() = default;

settings::settings(std::string progname, bool daemonize, bool use_syslog,
                   bool use_console, fs::path log_file,
                   const uint32_t log_file_max_size,
                   std::string transport_protocol, std::string bind_address,
                   uint32_t remote_port, fs::path pidfile, uint32_t workers,
                   uint32_t backlog_size, fs::path cfgfile,
                   std::list<namespace_def> defns)
    : m_progname(std::move(progname)), m_daemonize(daemonize),
      m_use_syslog(use_syslog), m_use_console(use_console),
      m_log_file(std::move(log_file)), m_log_file_max_size(log_file_max_size),
      m_transport_protocol(std::move(transport_protocol)),
      m_bind_address(std::move(bind_address)), m_remote_port(remote_port),
      m_daemon_pidfile(std::move(pidfile)), m_workers_in_pool(workers),
      m_backlog_size(backlog_size), m_config_file(std::move(cfgfile)),
      m_default_namespaces(std::move(defns)) {}

void
settings::load_defaults() {
    m_progname = defaults::progname;
    m_daemonize = defaults::daemonize;
    m_use_syslog = defaults::use_syslog;
    m_use_console = defaults::use_console;
    m_log_file = defaults::log_file;
    m_log_file_max_size = defaults::log_file_max_size;
    m_transport_protocol = defaults::transport_protocol;
    m_bind_address = defaults::bind_address;
    m_remote_port = defaults::remote_port;
    m_daemon_pidfile = defaults::pidfile;
    m_workers_in_pool = defaults::workers_in_pool;
    m_backlog_size = defaults::backlog_size;
    m_config_file = defaults::config_file;
    m_default_namespaces.clear();
}

void
settings::load_from_file(const fs::path& filename) {
    file_options::options_map opt_map;
    file_options::parse_yaml_file(filename, config::valid_options, opt_map);

    // load global settings
    const auto& gsettings = opt_map.get_as<file_options::options_group>(
            keywords::global_settings);

    m_progname = defaults::progname;
    m_use_syslog = gsettings.get_as<bool>(keywords::use_syslog);
    m_use_console = defaults::use_console;

    if(gsettings.has(keywords::log_file)) {
        m_log_file = gsettings.get_as<fs::path>(keywords::log_file);
    }

    if(gsettings.has(keywords::log_file_max_size)) {
        m_log_file_max_size =
                gsettings.get_as<uint32_t>(keywords::log_file_max_size);
    }

    m_transport_protocol =
            gsettings.get_as<std::string>(keywords::transport_protocol);
    m_bind_address = gsettings.get_as<std::string>(keywords::bind_address);
    m_remote_port = gsettings.get_as<uint32_t>(keywords::remote_port);
    m_daemon_pidfile = gsettings.get_as<fs::path>(keywords::pidfile);
    m_workers_in_pool = gsettings.get_as<uint32_t>(keywords::workers);
    m_backlog_size = defaults::backlog_size;

    // load definitions for default namespaces
    //    const auto& namespaces =
    //            opt_map.get_as<file_options::options_list>(keywords::namespaces);
    //
    //    for(const auto& nsdef : namespaces) {
    //        m_default_namespaces.emplace_back(
    //                nsdef.get_as<std::string>(keywords::nsid),
    //                nsdef.get_as<bool>(keywords::track_contents),
    //                nsdef.get_as<fs::path>(keywords::mountpoint),
    //                nsdef.get_as<std::string>(keywords::type),
    //                nsdef.get_as<uint64_t>(keywords::capacity),
    //                nsdef.get_as<std::string>(keywords::visibility));
    //    }
}

std::string
settings::to_string() const {
    std::string str =
            std::string("settings {\n") + "  m_progname: " + m_progname +
            ",\n" + "  m_daemonize: " + (m_daemonize ? "true" : "false") +
            ",\n" + "  m_use_syslog: " + (m_use_syslog ? "true" : "false") +
            ",\n" + "  m_use_console: " + (m_use_console ? "true" : "false") +
            ",\n" + "  m_log_file: " + m_log_file.string() + ",\n" +
            "  m_log_file_max_size: " + std::to_string(m_log_file_max_size) +
            ",\n" + "  m_bind_address: " + m_bind_address + ",\n" +
            "  m_remote_port: " + std::to_string(m_remote_port) + ",\n" +
            "  m_pidfile: " + m_daemon_pidfile.string() + ",\n" +
            "  m_workers: " + std::to_string(m_workers_in_pool) + ",\n" +
            "  m_backlog_size: " + std::to_string(m_backlog_size) + ",\n" +
            "  m_config_file: " + m_config_file.string() + ",\n" + "};";
    // TODO: add m_default_namespaces
    return str;
}

std::string
settings::progname() const {
    return m_progname;
}

void
settings::progname(const std::string& progname) {
    m_progname = progname;
}

bool
settings::daemonize() const {
    return m_daemonize;
}

void
settings::daemonize(bool daemonize) {
    m_daemonize = daemonize;
}

bool
settings::use_syslog() const {
    return m_use_syslog;
}

void
settings::use_syslog(bool use_syslog) {
    m_use_syslog = use_syslog;
}

bool
settings::use_console() const {
    return m_use_console;
}

void
settings::use_console(bool use_console) {
    m_use_console = use_console;
}

fs::path
settings::log_file() const {
    return m_log_file;
}

void
settings::log_file(const fs::path& log_file) {
    m_log_file = log_file;
}

uint32_t
settings::log_file_max_size() const {
    return m_log_file_max_size;
}

void
settings::log_file_max_size(uint32_t log_file_max_size) {
    m_log_file_max_size = log_file_max_size;
}

std::string
settings::transport_protocol() const {
    return m_transport_protocol;
}
void
settings::transport_protocol(const std::string& transport_protocol) {
    m_transport_protocol = transport_protocol;
}

std::string
settings::bind_address() const {
    return m_bind_address;
}

void
settings::bind_address(const std::string& bind_address) {
    m_bind_address = bind_address;
}

in_port_t
settings::remote_port() const {
    return m_remote_port;
}

void
settings::remote_port(in_port_t remote_port) {
    m_remote_port = remote_port;
}

fs::path
settings::pidfile() const {
    return m_daemon_pidfile;
}

void
settings::pidfile(const fs::path& pidfile) {
    m_daemon_pidfile = pidfile;
}

uint32_t
settings::workers_in_pool() const {
    return m_workers_in_pool;
}

void
settings::workers_in_pool(uint32_t workers_in_pool) {
    m_workers_in_pool = workers_in_pool;
}

uint32_t
settings::backlog_size() const {
    return m_backlog_size;
}

void
settings::backlog_size(uint32_t backlog_size) {
    m_backlog_size = backlog_size;
}

fs::path
settings::config_file() const {
    return m_config_file;
}

void
settings::config_file(const fs::path& config_file) {
    m_config_file = config_file;
}

std::list<namespace_def>
settings::default_namespaces() const {
    return m_default_namespaces;
}

void
settings::default_namespaces(
        const std::list<namespace_def>& default_namespaces) {
    m_default_namespaces = default_namespaces;
}

} // namespace scord::config
