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

#ifndef CARGO_WORKER_OPS_HPP
#define CARGO_WORKER_OPS_HPP

#include <memory>
#include <boost/mpi.hpp>
#include <filesystem>
#include "proto/mpi/message.hpp"
#include "cargo.hpp"

namespace cargo {

/**
 * Interface for transfer operations
 */
class operation {

public:
    static std::unique_ptr<operation>
    make_operation(cargo::tag t, boost::mpi::communicator workers,
                   std::filesystem::path input_path,
                   std::filesystem::path output_path);

    virtual ~operation() = default;

    virtual cargo::error_code
    operator()() const = 0;
};

} // namespace cargo

#endif // CARGO_WORKER_OPS_HPP
