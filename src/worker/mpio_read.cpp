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


#include "mpio_read.hpp"
#include "mpioxx.hpp"
#include "memory.hpp"
#include <thread>

namespace cargo {

mpio_read::mpio_read(mpi::communicator workers,
                     std::filesystem::path input_path,
                     std::filesystem::path output_path)
    : m_workers(std::move(workers)), m_input_path(std::move(input_path)),
      m_output_path(std::move(output_path)) {}

cargo::error_code
mpio_read::operator()() {

    using posix_file::views::all_of;
    using posix_file::views::as_blocks;
    using posix_file::views::strided;
    m_status = error_code::transfer_in_progress;
    try {
        const auto input_file = mpioxx::file::open(
                m_workers, m_input_path, mpioxx::file_open_mode::rdonly);

        mpioxx::offset file_size = input_file.size();
        std::size_t block_size = 512 * 1024u;

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

        m_buffer.resize(blocks_per_rank * block_size);

        m_buffer_regions.reserve(blocks_per_rank);

        for(std::size_t i = 0; i < blocks_per_rank; ++i) {
            m_buffer_regions.emplace_back(m_buffer.data() + i * block_size,
                                          block_size);
        }

        MPI_Datatype datatype = block_type;

        // step2. parallel read data into buffers
        if(const auto ec = MPI_File_read_all(input_file, m_buffer.data(),
                                             static_cast<int>(blocks_per_rank),
                                             datatype, MPI_STATUS_IGNORE);
           ec != MPI_SUCCESS) {
            LOGGER_ERROR("MPI_File_read_all() failed: {}",
                         mpi::error_string(ec));
            return make_mpi_error(ec);
        }

        // step3. POSIX write data
        m_output_file = std::make_unique<posix_file::file>(
                posix_file::create(m_output_path, O_WRONLY, S_IRUSR | S_IWUSR));

        m_output_file->fallocate(0, 0, file_size);


        m_workers_size = workers_size;
        m_workers_rank = workers_rank;
        m_block_size = block_size;


    } catch(const mpioxx::io_error& e) {
        LOGGER_ERROR("{}() failed: {}", e.where(), e.what());
        m_status = make_mpi_error(e.error_code());
        return make_mpi_error(e.error_code());
    } catch(const posix_file::io_error& e) {
        LOGGER_ERROR("{}() failed: {}", e.where(), e.what());
        m_status = make_system_error(e.error_code());
        return make_system_error(e.error_code());
    } catch(const std::system_error& e) {
        LOGGER_ERROR("Unexpected system error: {}", e.what());
        m_status = make_system_error(e.code().value());
        return make_system_error(e.code().value());
    } catch(const std::exception& e) {
        LOGGER_ERROR("Unexpected exception: {}", e.what());
        m_status = error_code::other;
        return error_code::other;
    }
    m_status = error_code::transfer_in_progress;
    return error_code::transfer_in_progress;
}

int
mpio_read::progress(int ongoing_index) {

    using posix_file::views::all_of;
    using posix_file::views::as_blocks;
    using posix_file::views::strided;
    try {
        int index = 0;
        m_status = error_code::transfer_in_progress;
        for(const auto& file_range :
            all_of(posix_file::file{m_input_path}) | as_blocks(m_block_size) |
                    strided(m_workers_size, m_workers_rank)) {
            if(index < ongoing_index) {
                ++index;
                continue;
            } else {
                if(index > ongoing_index) {
                    return index;
                }
            }

            assert(m_buffer_regions[index].size() >= file_range.size());
            auto start = std::chrono::steady_clock::now();
            m_output_file->pwrite(m_buffer_regions[index], file_range.offset(),
                                  file_range.size());
            auto end = std::chrono::steady_clock::now();
            // Send transfer bw
            double elapsed_seconds =
                    std::chrono::duration_cast<std::chrono::duration<double>>(
                            end - start)
                            .count();
            if((elapsed_seconds) > 0) {
                bw((m_block_size / (1024.0 * 1024.0)) / (elapsed_seconds));
                LOGGER_INFO("BW (write) Update: {} / {} = {} mb/s [ Sleep {} ]",
                            m_block_size / 1024.0, elapsed_seconds, bw(),
                            sleep_value());
            }
            // Do sleep
            std::this_thread::sleep_for(sleep_value());

            ++index;
        }
    } catch(const mpioxx::io_error& e) {
        LOGGER_ERROR("{}() failed: {}", e.where(), e.what());
        m_status = make_mpi_error(e.error_code());
        return -1;
    } catch(const posix_file::io_error& e) {
        LOGGER_ERROR("{}() failed: {}", e.where(), e.what());
        m_status = make_system_error(e.error_code());
        return -1;
    } catch(const std::system_error& e) {
        LOGGER_ERROR("Unexpected system error: {}", e.what());
        m_status = make_system_error(e.code().value());
        return -1;
    } catch(const std::exception& e) {
        LOGGER_ERROR("Unexpected exception: {}", e.what());
        m_status = error_code::other;
        return -1;
    }

    m_status = error_code::success;
    return -1;
}

// This needs to be go through different phases...
cargo::error_code
mpio_read::progress() const {
    return m_status;
}

} // namespace cargo