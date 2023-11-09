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

#include <logger/logger.hpp>
#include "sequential.hpp"


namespace cargo {

cargo::error_code
seq_operation::operator()() {
    LOGGER_CRITICAL("{}: to be implemented", __FUNCTION__);
    m_status = cargo::error_code::not_implemented;
    return cargo::error_code::not_implemented;
}

cargo::error_code
seq_operation::progress() const {
    return m_status;
}

int
seq_operation::progress(int ongoing_index) {
    ongoing_index++;
    m_status = cargo::error_code::not_implemented;
    return -1;
}

} // namespace cargo
