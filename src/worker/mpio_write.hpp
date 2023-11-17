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

#include <posix_file/file.hpp>
#include <posix_file/views.hpp>
#include "ops.hpp"
#include "memory.hpp"

namespace mpi = boost::mpi;

namespace cargo {

class mpio_write : public operation {

public:
    mpio_write(mpi::communicator workers, std::filesystem::path input_path,
               std::filesystem::path output_path, std::uint64_t block_size)
        : m_workers(std::move(workers)), m_input_path(std::move(input_path)),
          m_output_path(std::move(output_path)), m_kb_size(std::move(block_size)) {}

    cargo::error_code
    operator()() final;

    cargo::error_code
    progress() const final;

    int
    progress(int ongoing_index) final;


private:
    mpi::communicator m_workers;
    std::filesystem::path m_input_path;
    std::filesystem::path m_output_path;

    cargo::error_code m_status;


    std::unique_ptr<posix_file::file> m_input_file;
    int m_workers_size;
    int m_workers_rank;
    std::size_t m_block_size;
    std::size_t m_file_size;
    int m_total_blocks;

    memory_buffer m_buffer;
    std::vector<buffer_region> m_buffer_regions;
    std::size_t m_bytes_per_rank;
    std::uint64_t m_kb_size;
};

} // namespace cargo

#endif // CARGO_WORKER_MPIO_WRITE_HPP
