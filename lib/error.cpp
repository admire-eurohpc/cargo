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

#include <system_error>
#include <boost/mpi/error_string.hpp>
#include "cargo/error.hpp"

namespace cargo {

[[nodiscard]] std::string
error_code::message() const {

    switch(m_category) {
        case error_category::generic_error:
            switch(m_value) {
                case error_value::success:
                    return "success";
                case error_value::snafu:
                    return "snafu";
                case error_value::not_implemented:
                    return "not implemented";
                default:
                    return "unknown error";
            }
        case error_category::system_error: {
            std::error_code std_ec =
                    std::make_error_code(static_cast<std::errc>(m_value));
            return std_ec.message();
        }
        case error_category::mpi_error:
            return boost::mpi::error_string(static_cast<int>(m_value));
        default:
            return "unknown error category";
    }
};

} // namespace cargo