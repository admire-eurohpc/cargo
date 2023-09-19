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

#include "ops.hpp"
#include "mpio_read.hpp"
#include "mpio_write.hpp"
#include "sequential.hpp"

namespace mpi = boost::mpi;

namespace cargo {

std::unique_ptr<operation>
operation::make_operation(cargo::tag t, mpi::communicator workers,
                          std::filesystem::path input_path,
                          std::filesystem::path output_path) {
    using cargo::tag;
    switch(t) {
        case tag::pread:
            return std::make_unique<mpio_read>(std::move(workers),
                                               std::move(input_path),
                                               std::move(output_path));
        case tag::pwrite:
            return std::make_unique<mpio_write>(std::move(workers),
                                                std::move(input_path),
                                                std::move(output_path));
        case tag::sequential:
            return std::make_unique<seq_operation>(std::move(workers),
                                                   std::move(input_path),
                                                   std::move(output_path));
        default:
            return {};
    }
}

} // namespace cargo
