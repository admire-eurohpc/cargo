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

#include <boost/algorithm/string/case_conv.hpp>
#include <exception>
#include <stdexcept>
#include <cstdint>
#include <string>
#include <filesystem>
#include <utils/utils.hpp>
#include "parsers.hpp"

namespace fs = std::filesystem;

namespace config::parsers {

bool
parse_bool(const std::string& name, const std::string& value) {

    if(value == "1" || boost::algorithm::to_lower_copy(value) == "true") {
        return true;
    }

    if(value == "0" || boost::algorithm::to_lower_copy(value) == "false") {
        return false;
    }

    throw std::invalid_argument("Value provided for option '" + name +
                                "' is not boolean");
}

uint32_t
parse_number(const std::string& name, const std::string& value) {

    int32_t optval = 0;

    try {
        optval = std::stoi(value);
    } catch(...) {
        throw std::invalid_argument("Value provided for option '" + name +
                                    "' is not a number");
    }

    if(optval <= 0) {
        throw std::invalid_argument("Value provided for option '" + name +
                                    "' must be greater than zero");
    }

    return static_cast<uint32_t>(optval);
}

fs::path
parse_path(const std::string& name, const std::string& value) {

    (void) name;

    return {value};
}

fs::path
parse_existing_path(const std::string& name, const std::string& value) {

    if(!fs::exists(value)) {
        throw std::invalid_argument("Path '" + value + "' in option '" + name +
                                    "' does not exist");
    }

    return {value};
}

uint64_t
parse_capacity(const std::string& name, const std::string& value) {

    try {
        return utils::parse_size(value);
    } catch(const std::exception& e) {
        throw std::invalid_argument("Value provided in option '" + name +
                                    "' is invalid");
    }
}

} // namespace config::parsers
