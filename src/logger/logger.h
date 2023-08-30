/******************************************************************************
 * Copyright 2021-2023, Barcelona Supercomputing Center (BSC), Spain
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

#ifndef SCORD_LOGGER_H
#define SCORD_LOGGER_H

#include "macros.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOGGER_MSG_MAX_LEN 2048

enum logger_type {
    CONSOLE_LOGGER,
    CONSOLE_COLOR_LOGGER,
    FILE_LOGGER,
    SYSLOG_LOGGER
};
enum logger_level { info, debug, warn, error, critical };

/**
 * Initialize a global logger.
 *
 * Valid logger types:
 *  - console
 *  - console color
 *  - file
 *
 * @param ident The identifier that should be used when emitting messages
 * @param type The type of logger desired.
 * @param log_file The path to the log file where messages should be written.
 */
void
setup_logger(const char* ident, enum logger_type type, const char* log_file);

/**
 * Emit a message.
 *
 * @param level The level for the message.
 * @param fmt A vprintf()-compatible format string.
 * @param ... A sequence of arguments corresponding to the format string.
 */
void
logger_log(enum logger_level level, const char* fmt, ...);

/**
 * Destroy the global logger.
 */
void
destroy_logger();

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // SCORD_LOGGER_H
