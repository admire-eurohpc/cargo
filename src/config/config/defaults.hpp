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

#ifndef SCORD_CONFIG_DEFAULTS_HPP
#define SCORD_CONFIG_DEFAULTS_HPP

#include <cstdint>
#include <netinet/in.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace scord::config::defaults {

extern const char* progname;
extern const bool daemonize;
extern const bool use_syslog;
extern const bool use_console;
extern const fs::path log_file;
extern const uint32_t log_file_max_size;
extern const char* transport_protocol;
extern const char* bind_address;
extern const in_port_t remote_port;
extern const char* pidfile;
extern const uint32_t workers_in_pool;
extern const char* staging_directory;
extern const uint32_t backlog_size;
extern const char* config_file;

} // namespace scord::config::defaults

#endif /* SCORD_CONFIG_DEFAULTS_HPP */
