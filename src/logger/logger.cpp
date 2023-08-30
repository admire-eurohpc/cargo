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

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/syslog_sink.h>
#include <utility>
#include <cstdarg>
#include "logger.hpp"
#include "logger.h"

////////////////////////////////////////////////////////////////////////////////
// Function implementations for the C API
////////////////////////////////////////////////////////////////////////////////

void
logger_setup(const char* ident, logger_type type, const char* log_file) {

    constexpr auto get_cxx_type = [](logger_type t) {
        switch(t) {
            case CONSOLE_LOGGER:
                return logger::logger_type::console;
            case CONSOLE_COLOR_LOGGER:
                return logger::logger_type::console_color;
            case FILE_LOGGER:
                return logger::logger_type::file;
            case SYSLOG_LOGGER:
                return logger::logger_type::syslog;
            default:
                return logger::logger_type::console;
        }
    };

    logger::create_default_logger(
            logger::logger_config{ident, get_cxx_type(type), log_file});
}

void
logger_log(enum logger_level level, const char* fmt, ...) {

    using logger::logger_base;

    if(const auto logger = logger_base::get_default_logger(); logger) {

        std::array<char, LOGGER_MSG_MAX_LEN> msg; // NOLINT
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg.data(), msg.size(), fmt, args);
        va_end(args);

        switch(level) {
            case info:
                logger->info(msg.data());
                break;
            case debug:
                logger->debug(msg.data());
                break;
            case warn:
                logger->warn(msg.data());
                break;
            case error:
                logger->error(msg.data());
                break;
            case critical:
                logger->critical(msg.data());
                break;
        }
    }
}

void
logger_destroy() {
    using logger::logger_base;

    if(logger_base::get_default_logger()) {
        ::logger::destroy_default_logger();
    }
}

////////////////////////////////////////////////////////////////////////////////
// Function implementations for the C++ API
////////////////////////////////////////////////////////////////////////////////

namespace {

/**
 * @brief Creates a logger of the given type.
 *
 * @tparam Logger Type of the logger to create.
 * @param config Configuration for the logger.
 * @return std::shared_ptr<spdlog::logger> Pointer to the created logger.
 */
template <typename Logger>
std::shared_ptr<spdlog::logger>
create_logger(const logger::logger_config& config)
        requires(std::is_same_v<Logger, logger::sync_logger> ||
                 std::is_same_v<Logger, logger::async_logger>) {

    const auto create_helper = [&config]() {
        switch(config.type()) {
            case logger::console: {
                if constexpr(std::is_same_v<Logger, logger::sync_logger>) {
                    return spdlog::stdout_logger_st(config.ident());
                }
                return spdlog::stdout_logger_mt<spdlog::async_factory>(
                        config.ident());
            }
            case logger::console_color:
                if constexpr(std::is_same_v<Logger, logger::sync_logger>) {
                    return spdlog::stdout_color_st(config.ident());
                }
                return spdlog::stdout_color_mt<spdlog::async_factory>(
                        config.ident());
            case logger::file:
                if constexpr(std::is_same_v<Logger, logger::sync_logger>) {
                    return spdlog::basic_logger_st(
                            config.ident(), config.log_file().value_or(""),
                            true);
                }
                return spdlog::basic_logger_mt<spdlog::async_factory>(
                        config.ident(), config.log_file().value_or(""), true);
            case logger::syslog:
                if constexpr(std::is_same_v<Logger, logger::sync_logger>) {
                    return spdlog::syslog_logger_st("syslog", config.ident(),
                                                    LOG_PID);
                }
                return spdlog::syslog_logger_mt("syslog", config.ident(),
                                                LOG_PID);
            default:
                throw std::invalid_argument("Unknown logger type");
        }
    };

    try {
        auto logger = create_helper();
        assert(logger != nullptr);
        logger->set_pattern(logger::default_pattern);

#ifdef LOGGER_ENABLE_DEBUG
        logger->set_level(spdlog::level::debug);
#endif
        spdlog::drop_all();
        return logger;
    } catch(const spdlog::spdlog_ex& ex) {
        throw std::runtime_error("logger initialization failed: " +
                                 std::string(ex.what()));
    }
}

} // namespace

namespace logger {

logger_base::logger_base(logger::logger_config config)
    : m_config(std::move(config)),
      m_internal_logger(::create_logger<async_logger>(m_config)) {}

const logger_config&
logger_base::config() const {
    return m_config;
}

std::shared_ptr<logger_base>&
logger_base::get_default_logger() {
    static std::shared_ptr<logger_base> s_global_logger;
    return s_global_logger;
}

void
logger_base::enable_debug() const {
    m_internal_logger->set_level(spdlog::level::debug);
}

void
logger_base::flush() {
    m_internal_logger->flush();
}

async_logger::async_logger(const logger_config& config) : logger_base(config) {
    try {
        m_internal_logger = ::create_logger<async_logger>(config);
    } catch(const spdlog::spdlog_ex& ex) {
        throw std::runtime_error("logger initialization failed: " +
                                 std::string(ex.what()));
    }
}

sync_logger::sync_logger(const logger_config& config) : logger_base(config) {
    try {
        m_internal_logger = ::create_logger<sync_logger>(config);
    } catch(const spdlog::spdlog_ex& ex) {
        throw std::runtime_error("logger initialization failed: " +
                                 std::string(ex.what()));
    }
}

} // namespace logger
