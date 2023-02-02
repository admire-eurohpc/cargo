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

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace utils {

template <typename T>
class singleton {
public:
    static T&
    instance() {
        static T s_instance;
        return s_instance;
    };

    singleton(const singleton&) = delete;
    singleton(singleton&&) = delete;
    singleton&
    operator=(const singleton&) = delete;
    singleton&
    operator=(singleton&&) = delete;

protected:
    struct token {};
    singleton() = default;
    ~singleton() = default;
};

uint64_t
parse_size(const std::string& str);

template <typename T>
std::string
n2hexstr(T i, bool zero_pad = false) {
    std::stringstream ss;

    if(zero_pad) {
        ss << std::setfill('0') << std::setw(sizeof(T) << 1);
    }

    ss << std::showbase << std::hex << i;
    return ss.str();
}

std::filesystem::path
lexical_normalize(const std::filesystem::path& pathname,
                  bool as_directory = false);

std::filesystem::path
remove_trailing_separator(const std::filesystem::path& pathname);

std::filesystem::path
remove_leading_separator(const std::filesystem::path& pathname);

} // namespace utils

#endif // UTILS_HPP
