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

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <fmt/ostream.h>
#include <filesystem>
#include <sstream>

#if FMT_VERSION < 50000
namespace fmt {
template <typename T>
inline const void*
ptr(const T* p) {
    return p;
}
} // namespace fmt
#endif // FMT_VERSION

namespace fs = std::filesystem;

class logger {

public:
    logger(const std::string& ident, const std::string& type,
           const fs::path& log_file = "") {

        try {

            if(type == "console") {
                m_internal_logger =
                        spdlog::stdout_logger_mt<spdlog::async_factory>(ident);
            } else if(type == "console color") {
                m_internal_logger =
                        spdlog::stdout_color_mt<spdlog::async_factory>(ident);
            } else if(type == "file") {
                m_internal_logger =
                        spdlog::basic_logger_mt<spdlog::async_factory>(
                                ident, log_file.string(), true);

            }
#ifdef SPDLOG_ENABLE_SYSLOG
            else if(type == "syslog") {
                m_internal_logger =
                        spdlog::syslog_logger("syslog", ident, LOG_PID);
            }
#endif
            else {
                throw std::invalid_argument("Unknown logger type: '" + type +
                                            "'");
            }

            assert(m_internal_logger != nullptr);

            // %Y - Year in 4 digits
            // %m - month 1-12
            // %d - day 1-31
            // %T - ISO 8601 time format (HH:MM:SS)
            // %f - microsecond part of the current second
            // %E - epoch (microseconds precision)
            // %n - logger's name
            // %t - thread id
            // %l - log level
            // %v - message
            // m_internal_logger->set_pattern("[%Y-%m-%d %T.%f] [%E] [%n] [%t]
            // [%l] %v");
            m_internal_logger->set_pattern(
                    "%^[%Y-%m-%d %T.%f] [%n] [%t] [%l]%$ %v");

#ifdef __LOGGER_ENABLE_DEBUG__
            enable_debug();
#endif

            spdlog::drop_all();

            // globally register the logger so that it can be accessed
            // using spdlog::get(logger_name)
            // spdlog::register_logger(m_internal_logger);
        } catch(const spdlog::spdlog_ex& ex) {
            throw std::runtime_error("logger initialization failed: " +
                                     std::string(ex.what()));
        }
    }

    logger(const logger& /*rhs*/) = delete;
    logger&
    operator=(const logger& /*rhs*/) = delete;
    logger(logger&& /*other*/) = default;
    logger&
    operator=(logger&& /*other*/) = default;

    ~logger() {
        spdlog::drop_all();
    }

    // the following static functions can be used to interact
    // with a globally registered logger instance

    template <typename... Args>
    static inline void
    create_global_logger(Args&&... args) {
        global_logger() = std::make_shared<logger>(args...);
    }

    static inline void
    register_global_logger(logger&& lg) {
        global_logger() = std::make_shared<logger>(std::move(lg));
    }

    static inline std::shared_ptr<logger>&
    get_global_logger() {
        return global_logger();
    }

    static inline void
    destroy_global_logger() {
        global_logger().reset();
    }

    // the following member functions can be used to interact
    // with a specific logger instance
    inline void
    enable_debug() const {
        m_internal_logger->set_level(spdlog::level::debug);
    }

    inline void
    flush() {
        m_internal_logger->flush();
    }

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

    static inline std::string
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
    static inline std::string
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

private:
    static std::shared_ptr<logger>&
    global_logger() {
        static std::shared_ptr<logger> s_global_logger;
        return s_global_logger;
    }

private:
    std::shared_ptr<spdlog::logger> m_internal_logger;
    std::string m_type;
};

// some macros to make it more convenient to use the global logger
#define LOGGER_INIT(...)                                                       \
    do {                                                                       \
        logger::create_global_logger(__VA_ARGS__);                             \
    } while(0);

#define LOGGER_INFO(...)                                                       \
    do {                                                                       \
        if(logger::get_global_logger()) {                                      \
            logger::get_global_logger()->info(__VA_ARGS__);                    \
        }                                                                      \
    } while(0);


#ifdef __LOGGER_ENABLE_DEBUG__

#define LOGGER_DEBUG(...)                                                      \
    do {                                                                       \
        if(logger::get_global_logger()) {                                      \
            logger::get_global_logger()->debug(__VA_ARGS__);                   \
        }                                                                      \
    } while(0);

#define LOGGER_FLUSH()                                                         \
    do {                                                                       \
        if(logger::get_global_logger()) {                                      \
            logger::get_global_logger()->flush();                              \
        }                                                                      \
    } while(0);

#else

#define LOGGER_DEBUG(...)                                                      \
    do {                                                                       \
    } while(0);
#define LOGGER_FLUSH()                                                         \
    do {                                                                       \
    } while(0);

#endif // __LOGGER_ENABLE_DEBUG__

#define LOGGER_WARN(...)                                                       \
    do {                                                                       \
        if(logger::get_global_logger()) {                                      \
            logger::get_global_logger()->warn(__VA_ARGS__);                    \
        }                                                                      \
    } while(0);

#define LOGGER_ERROR(...)                                                      \
    do {                                                                       \
        if(logger::get_global_logger()) {                                      \
            logger::get_global_logger()->error(__VA_ARGS__);                   \
        }                                                                      \
    } while(0);

#define LOGGER_ERRNO(...)                                                      \
    do {                                                                       \
        if(logger::get_global_logger()) {                                      \
            logger::get_global_logger()->error_errno(__VA_ARGS__);             \
        }                                                                      \
    } while(0);

#define LOGGER_CRITICAL(...)                                                   \
    do {                                                                       \
        if(logger::get_global_logger()) {                                      \
            logger::get_global_logger()->critical(__VA_ARGS__);                \
        }                                                                      \
    } while(0);

#endif /* LOGGER_HPP */
