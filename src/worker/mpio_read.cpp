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

#include <posix_file/file.hpp>
#include <posix_file/views.hpp>
#include "mpio_read.hpp"
#include "mpioxx.hpp"
#include "memory.hpp"

namespace cargo {

mpio_read::mpio_read(mpi::communicator workers,
                     std::filesystem::path input_path,
                     std::filesystem::path output_path)
    : m_workers(std::move(workers)), m_input_path(std::move(input_path)),
      m_output_path(std::move(output_path)) {}

cargo::error_code
mpio_read::operator()() const {

    using posix_file::views::all_of;
    using posix_file::views::as_blocks;
    using posix_file::views::strided;

    try {
        const auto input_file = mpioxx::file::open(
                m_workers, m_input_path, mpioxx::file_open_mode::rdonly);

        mpioxx::offset file_size = input_file.size();
        std::size_t block_size = 512u;

        // create block type
        MPI_Datatype block_type;
        MPI_Type_contiguous(static_cast<int>(block_size), MPI_BYTE,
                            &block_type);
        MPI_Type_commit(&block_type);

        // compute the number of blocks in the file
        int total_blocks = static_cast<int>(file_size / block_size);

        if(file_size % block_size != 0) {
            ++total_blocks;
        }

        const auto workers_size = m_workers.size();
        const auto workers_rank = m_workers.rank();

        // create file type
        MPI_Datatype file_type;
        /*
         * count: number of blocks in the type
         * blocklen: number of elements in each block
         * stride: number of elements between start of each block
         */
        MPI_Type_vector(/* count: */ total_blocks, /* blocklength: */ 1,
                        /* stride: */ workers_size, /* oldtype: */ block_type,
                        &file_type);
        MPI_Type_commit(&file_type);

        MPI_Offset disp = workers_rank * block_size;
        MPI_Datatype etype = block_type;
        MPI_Datatype filetype = file_type;

        if(const auto ec = MPI_File_set_view(input_file, disp, etype, filetype,
                                             "native", MPI_INFO_NULL);
           ec != MPI_SUCCESS) {
            LOGGER_ERROR("MPI_File_set_view() failed: {}",
                         mpi::error_string(ec));
            return make_mpi_error(ec);
        }

        // find how many blocks this rank is responsible for
        std::size_t blocks_per_rank = total_blocks / workers_size;

        if(int64_t n = total_blocks % workers_size;
           n != 0 && workers_rank < n) {
            ++blocks_per_rank;
        }

        // step 1. acquire buffers
        memory_buffer buffer;
        buffer.resize(blocks_per_rank * block_size);

        std::vector<buffer_region> buffer_regions;
        buffer_regions.reserve(blocks_per_rank);

        for(std::size_t i = 0; i < blocks_per_rank; ++i) {
            buffer_regions.emplace_back(buffer.data() + i * block_size,
                                        block_size);
        }

        MPI_Datatype datatype = block_type;

        // step2. parallel read data into buffers
        if(const auto ec = MPI_File_read_all(input_file, buffer.data(),
                                             static_cast<int>(blocks_per_rank),
                                             datatype, MPI_STATUS_IGNORE);
           ec != MPI_SUCCESS) {
            LOGGER_ERROR("MPI_File_read_all() failed: {}",
                         mpi::error_string(ec));
            return make_mpi_error(ec);
        }

        // step3. POSIX write data
        const auto output_file =
                posix_file::create(m_output_path, O_WRONLY, S_IRUSR | S_IWUSR);

        output_file.fallocate(0, 0, file_size);

        int index = 0;
        for(const auto& file_range :
            all_of(posix_file::file{m_input_path}) | as_blocks(block_size) |
                    strided(workers_size, workers_rank)) {
            assert(buffer_regions[index].size() >= file_range.size());
            output_file.pwrite(buffer_regions[index], file_range.offset(),
                               file_range.size());

            ++index;
        }
    } catch(const mpioxx::io_error& e) {
        LOGGER_ERROR("{}() failed: {}", e.where(), e.what());
        return make_mpi_error(e.error_code());
    } catch(const posix_file::io_error& e) {
        LOGGER_ERROR("{}() failed: {}", e.where(), e.what());
        return make_system_error(e.error_code());
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }

    return error_code::success;
}

} // namespace cargo