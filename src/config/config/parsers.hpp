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

#ifndef CONFIG_PARSERS_HPP
#define CONFIG_PARSERS_HPP

#include <cstdint>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace config::parsers {

bool
parse_bool(const std::string& name, const std::string& value);
uint32_t
parse_number(const std::string& name, const std::string& value);
fs::path
parse_path(const std::string& name, const std::string& value);
fs::path
parse_existing_path(const std::string& name, const std::string& value);
uint64_t
parse_capacity(const std::string& name, const std::string& value);

} // namespace config::parsers

#endif // CONFIG_PARSERS_HPP
