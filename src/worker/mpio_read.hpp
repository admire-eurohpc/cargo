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

#ifndef CARGO_WORKER_MPIO_READ_HPP
#define CARGO_WORKER_MPIO_READ_HPP

#include "ops.hpp"
#include "memory.hpp"
#include <posix_file/file.hpp>
#include <posix_file/views.hpp>

namespace mpi = boost::mpi;

namespace cargo {

class mpio_read : public operation {

public:
    mpio_read(mpi::communicator workers, std::filesystem::path input_path,
              std::filesystem::path output_path, std::uint64_t block_size,
              FSPlugin::type fs_i_type, FSPlugin::type m_fs_o_type);

    cargo::error_code
    operator()() final;

    cargo::error_code
    progress() const final;

    int
    progress(int ongoing_index) final;

    std::string
    output_path() const {
        return m_output_path;
    }

    std::string
    input_path() const {
        return m_input_path;
    }

private:
    mpi::communicator m_workers;
    cargo::error_code m_status;
    std::filesystem::path m_input_path{};
    std::filesystem::path m_output_path{};
    std::unique_ptr<posix_file::file> m_output_file;
    int m_workers_size;
    int m_workers_rank;
    std::size_t m_block_size;
    memory_buffer m_buffer;
    std::vector<buffer_region> m_buffer_regions;
    std::uint64_t m_kb_size;
    FSPlugin::type m_fs_i_type;
    FSPlugin::type m_fs_o_type;
};

} // namespace cargo

#endif // CARGO_WORKER_MPIO_READ_HPP
