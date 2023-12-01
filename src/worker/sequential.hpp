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

#ifndef CARGO_WORKER_SEQUENTIAL_HPP
#define CARGO_WORKER_SEQUENTIAL_HPP

#include "ops.hpp"

namespace mpi = boost::mpi;

namespace cargo {

class seq_operation : public operation {

public:
    seq_operation(mpi::communicator comm, std::filesystem::path input_path,
                  std::filesystem::path output_path, std::uint64_t block_size, FSPlugin::type fs_type)
        : m_comm(std::move(comm)), m_input_path(std::move(input_path)),
          m_output_path(std::move(output_path)), m_kb_size(std::move(block_size)), m_fs_type(fs_type) {}

    cargo::error_code
    operator()() final;
    cargo::error_code
    progress() const;

       int
    progress(int ongoing_index ) final;

private:
    mpi::communicator m_comm;
    std::filesystem::path m_input_path;
    std::filesystem::path m_output_path;
    cargo::error_code m_status;
    std::uint64_t m_kb_size;
    FSPlugin::type m_fs_type;
};

} // namespace cargo

#endif // CARGO_WORKER_SEQUENTIAL_HPP
