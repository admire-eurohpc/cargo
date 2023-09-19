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

#ifndef CARGO_WORKER_MPIO_WRITE_HPP
#define CARGO_WORKER_MPIO_WRITE_HPP

#include "ops.hpp"

namespace mpi = boost::mpi;

namespace cargo {

class mpio_write : public operation {

public:
    mpio_write(mpi::communicator workers, std::filesystem::path input_path,
               std::filesystem::path output_path)
        : m_workers(std::move(workers)), m_input_path(std::move(input_path)),
          m_output_path(std::move(output_path)) {}

    cargo::error_code
    operator()() const final;

private:
    mpi::communicator m_workers;
    std::filesystem::path m_input_path;
    std::filesystem::path m_output_path;
};

} // namespace cargo

#endif // CARGO_WORKER_MPIO_WRITE_HPP
