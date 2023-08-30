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


#ifndef LOGGER_MACROS_H
#define LOGGER_MACROS_H

/* logger macros for C++ code */
#ifdef __cplusplus

#define LOGGER_INIT(...)                                                       \
    do {                                                                       \
        logger::create_default_logger(__VA_ARGS__);                            \
    } while(0);

#define LOGGER_INFO(...)                                                       \
    do {                                                                       \
        if(logger::get_default_logger()) {                                     \
            logger::get_default_logger()->info(__VA_ARGS__);                   \
        }                                                                      \
    } while(0);


#ifdef LOGGER_ENABLE_DEBUG

#define LOGGER_DEBUG(...)                                                      \
    do {                                                                       \
        if(logger::get_default_logger()) {                                     \
            logger::get_default_logger()->debug(__VA_ARGS__);                  \
        }                                                                      \
    } while(0);

#define LOGGER_FLUSH()                                                         \
    do {                                                                       \
        if(logger::get_default_logger()) {                                     \
            logger::get_default_logger()->flush();                             \
        }                                                                      \
    } while(0);

#else // ! LOGGER_ENABLE_DEBUG

#define LOGGER_DEBUG(...)                                                      \
    do {                                                                       \
    } while(0);
#define LOGGER_FLUSH()                                                         \
    do {                                                                       \
    } while(0);

#endif // LOGGER_ENABLE_DEBUG

#define LOGGER_WARN(...)                                                       \
    do {                                                                       \
        if(logger::get_default_logger()) {                                     \
            logger::get_default_logger()->warn(__VA_ARGS__);                   \
        }                                                                      \
    } while(0);

#define LOGGER_ERROR(...)                                                      \
    do {                                                                       \
        if(logger::get_default_logger()) {                                     \
            logger::get_default_logger()->error(__VA_ARGS__);                  \
        }                                                                      \
    } while(0);

#define LOGGER_ERRNO(...)                                                      \
    do {                                                                       \
        if(logger::get_default_logger()) {                                     \
            logger::get_default_logger()->error_errno(__VA_ARGS__);            \
        }                                                                      \
    } while(0);

#define LOGGER_CRITICAL(...)                                                   \
    do {                                                                       \
        if(logger::get_default_logger()) {                                     \
            logger::get_default_logger()->critical(__VA_ARGS__);               \
        }                                                                      \
    } while(0);

#else // ! __cplusplus

/* logger macros for C code */

// clang-format off
#define ARGSN(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define RSEQN() 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
// clang-format on
#define EXPAND(x)                x
#define COUNT_ARGS(...)          EXPAND(COUNT_ARGS_HELPER(__VA_ARGS__, RSEQN()))
#define COUNT_ARGS_HELPER(...)   EXPAND(ARGSN(__VA_ARGS__))
#define GET_NAME_HELPER(name, n) name##n
#define GET_NAME(name, n)        GET_NAME_HELPER(name, n)
#define GET_MACRO(func, ...)                                                   \
    GET_NAME(func, EXPAND(COUNT_ARGS(__VA_ARGS__)))(__VA_ARGS__)

#define LOGGER_LOG(level, ...)       GET_MACRO(LOGGER_LOG, level, __VA_ARGS__)
#define LOGGER_LOG2(level, msg)      logger_log(level, "%s", msg)
#define LOGGER_LOG3                  LOGGER_LOGN
#define LOGGER_LOG4                  LOGGER_LOGN
#define LOGGER_LOG5                  LOGGER_LOGN
#define LOGGER_LOG6                  LOGGER_LOGN
#define LOGGER_LOG7                  LOGGER_LOGN
#define LOGGER_LOG8                  LOGGER_LOGN
#define LOGGER_LOG9                  LOGGER_LOGN
#define LOGGER_LOG10                 LOGGER_LOGN
#define LOGGER_LOG11                 LOGGER_LOGN
#define LOGGER_LOG12                 LOGGER_LOGN
#define LOGGER_LOG13                 LOGGER_LOGN
#define LOGGER_LOG14                 LOGGER_LOGN
#define LOGGER_LOG15                 LOGGER_LOGN
#define LOGGER_LOG16                 LOGGER_LOGN
#define LOGGER_LOGN(level, fmt, ...) logger_log(level, fmt, __VA_ARGS__)

#define LOGGER_INFO(fmt, ...) LOGGER_LOG(info, fmt, ##__VA_ARGS__);

#ifdef LOGGER_ENABLE_DEBUG
#define LOGGER_DEBUG(fmt, ...) LOGGER_LOG(debug, fmt, ##__VA_ARGS__);
#endif

#define LOGGER_WARN(fmt, ...) LOGGER_LOG(warn, fmt, ##__VA_ARGS__);

#define LOGGER_ERROR(fmt, ...) LOGGER_LOG(error, fmt, ##__VA_ARGS__);

#endif

#define LOGGER_EVAL(expr, L1, L2, ...)                                         \
    do {                                                                       \
        if(expr) {                                                             \
            LOGGER_##L1(__VA_ARGS__);                                          \
        } else {                                                               \
            LOGGER_##L2(__VA_ARGS__);                                          \
        }                                                                      \
    } while(0);

#endif // LOGGER_MACROS_H
