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

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <spdlog/logger.h>
#include <fmt/ostream.h>
#include <filesystem>
#include <optional>
#include <sstream>

#include "macros.h"

#if FMT_VERSION < 50000
namespace fmt {
template <typename T>
inline const void*
ptr(const T* p) {
    return p;
}
} // namespace fmt
#endif // FMT_VERSION

namespace logger {

enum logger_type {
    console,
    console_color,
    file,
    syslog,
};

class logger_config {

public:
    logger_config() = default;

    explicit logger_config(std::string ident, logger_type type,
                           std::optional<std::filesystem::path> log_file = {})
        : m_ident(std::move(ident)), m_type(type),
          m_log_file(std::move(log_file)) {}

    const std::string&
    ident() const {
        return m_ident;
    }

    logger_type
    type() const {
        return m_type;
    }

    const std::optional<std::filesystem::path>&
    log_file() const {
        return m_log_file;
    }

private:
    std::string m_ident;
    logger_type m_type = console_color;
    std::optional<std::filesystem::path> m_log_file;
};


/**
 * @brief The default log pattern
 *
 * This is the default log pattern used by the logger.
 * It can be used to create new loggers with the same
 * configuration.
 *
 * The default log pattern is:
 *
 * @code
 * %^[%Y-%m-%d %T.%f] [%n] [%t] [%l]%$ %v
 * @endcode
 *
 * The output of the default log pattern is:
 *
 * @code
 * [2021-01-01 00:00:00.000000] [prog] [12345] [info] Message
 * @endcode
 *
 * Where:
 * - 2021-01-01 00:00:00.000000 is the current date and time
 * - prog is the name of the logger
 * - 12345 is the thread id
 * - info is the log level
 * - Message is the log message
 *
 * The following format specifiers are available:
 *   %Y - Year in 4 digits
 *   %m - month 1-12
 *   %d - day 1-31
 *   %T - ISO 8601 time format (HH:MM:SS)
 *   %f - microsecond part of the current second
 *   %E - epoch (microseconds precision)
 *   %n - logger's name
 *   %t - thread id
 *   %l - log level
 *   %v - message
 */
static constexpr auto default_pattern =
        "%^[%Y-%m-%d %T.%f] [%n] [%t] [%l]%$ %v";

/**
 * @brief The logger_base class
 *
 * This class is a wrapper around spdlog::logger, and provides common
 * functionality to all the logger implementations. It also provides a
 * default logger that can be used by the rest of the code by using the
 * static member function get_default_logger().
 *
 * @note This class should not be used directly. It is intended to serve as a
 *     base class for the different logger implementations.
 */
class logger_base {

protected:
    logger_base() = default;
    explicit logger_base(logger_config config);

public:
    logger_base(const logger_base& /*rhs*/) = delete;
    logger_base&
    operator=(const logger_base& /*rhs*/) = delete;

protected:
    logger_base(logger_base&& /*other*/) = default;
    logger_base&
    operator=(logger_base&& /*other*/) = default;
    ~logger_base() = default;

public:
    const logger_config&
    config() const;

    static std::shared_ptr<logger_base>&
    get_default_logger();

    void
    enable_debug() const;

    void
    flush();

    template <typename... Args>
    inline void
    info(fmt::format_string<Args...> fmt, Args&&... args) {
        m_internal_logger->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void
    debug(fmt::format_string<Args...> fmt, Args&&... args) {
        m_internal_logger->debug(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void
    warn(fmt::format_string<Args...> fmt, Args&&... args) {
        m_internal_logger->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void
    error(fmt::format_string<Args...> fmt, Args&&... args) {
        m_internal_logger->error(fmt, std::forward<Args>(args)...);
    }

    [[maybe_unused]] static inline std::string
    errno_message(int errno_value) {
        // 1024 should be more than enough for most locales
        constexpr const std::size_t MAX_ERROR_MSG = 1024;
        std::array<char, MAX_ERROR_MSG> errstr{};
        char* msg = strerror_r(errno_value, errstr.data(), MAX_ERROR_MSG);
        return std::string{msg};
    }

    template <typename... Args>
    inline void
    error_errno(fmt::format_string<Args...> fmt, Args&&... args) {

        const int saved_errno = errno;

        constexpr const std::size_t MAX_ERROR_MSG = 128;
        std::array<char, MAX_ERROR_MSG> errstr; // NOLINT

        fmt::string_view sv{fmt};
        std::string str_fmt{sv.begin(), sv.end()};
        str_fmt += ": {}";

        char* msg = strerror_r(saved_errno, errstr.data(), MAX_ERROR_MSG);

        m_internal_logger->error(fmt::runtime(str_fmt),
                                 std::forward<Args>(args)..., msg);
    }

    template <typename... Args>
    inline void
    critical(fmt::format_string<Args...> fmt, Args&&... args) {
        m_internal_logger->critical(fmt, std::forward<Args>(args)...);
    }

    template <typename T>
    inline void
    info(const T& msg) {
        m_internal_logger->info(msg);
    }

    template <typename T>
    inline void
    debug(const T& msg) {
        m_internal_logger->debug(msg);
    }

    template <typename T>
    inline void
    warn(const T& msg) {
        m_internal_logger->warn(msg);
    }

    template <typename T>
    inline void
    error(const T& msg) {
        m_internal_logger->error(msg);
    }

    template <typename T>
    inline void
    critical(const T& msg) {
        m_internal_logger->critical(msg);
    }

    template <typename... Args>
    [[deprecated]] [[maybe_unused]] static inline std::string
    build_message(Args&&... args) {

        // see:
        // 1.
        // https://stackoverflow.com/questions/27375089/what-is-the-easiest-way-to-print-a-variadic-parameter-pack-using-stdostream
        // 2.
        // https://stackoverflow.com/questions/25680461/variadic-template-pack-expansion/25683817#25683817

        std::stringstream ss;

        using expander = int[];
        (void) expander{0, (void(ss << std::forward<Args>(args)), 0)...};

        return ss.str();
    }

protected:
    logger_config m_config;
    std::shared_ptr<spdlog::logger> m_internal_logger;
};

/**
 * @brief Synchronous logger implementation
 *
 * This class is a wrapper around spdlog::logger. It provides
 * a synchronous interface to the spdlog logger.
 */
class sync_logger : public logger_base {
public:
    explicit sync_logger(const logger_config& config);
};

/**
 * @brief Asynchronous logger implementation
 *
 * This class is a wrapper around spdlog::async_logger. It
 * provides an asynchronous interface to the spdlog logger.
 */
class async_logger : public logger_base {
public:
    explicit async_logger(const logger_config& config);
};

// the following static functions can be used to interact
// with a globally registered async logger instance

/**
 * @brief Create a default logger instance
 *
 * @tparam Args variadic template parameter pack for the logger constructor
 * arguments
 * @param args arguments for the logger constructor
 */
template <typename... Args>
static inline void
create_default_logger(Args&&... args) {
    async_logger::get_default_logger() =
            std::make_shared<async_logger>(logger_config{args...});
}

/**
 * @brief Register an existing logger instance as the default logger
 *
 * @param config logger configuration
 */
[[maybe_unused]] static inline void
set_default_logger(async_logger&& lg) {
    async_logger::get_default_logger() =
            std::make_shared<async_logger>(std::move(lg));
}

/**
 * @brief Destroy the default logger instance
 */
[[maybe_unused]] static inline void
destroy_default_logger() {
    async_logger::get_default_logger().reset();
}

/**
 * @brief Get the default logger instance
 *
 * @return A shared pointer to the default logger instance
 */
[[maybe_unused]] static inline auto
get_default_logger() {
    return async_logger::get_default_logger();
}

/**
 * @brief Flush the default logger instance
 */
[[maybe_unused]] static inline void
flush_default_logger() {
    if(auto lg = async_logger::get_default_logger(); lg) {
        lg->flush();
    }
}

} // namespace logger

#endif /* LOGGER_HPP */
