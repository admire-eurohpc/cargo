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
#include <thread>

namespace cargo {

cargo::error_code
seq_operation::operator()() {
    using posix_file::views::all_of;
    using posix_file::views::as_blocks;
    using posix_file::views::strided;
    m_status = error_code::transfer_in_progress;
    try {

        const auto workers_size = m_workers.size();
        const auto workers_rank = m_workers.rank();
        std::size_t block_size = m_kb_size * 1024u;
        m_input_file = std::make_unique<posix_file::file>(
                posix_file::open(m_input_path, O_RDONLY, 0, m_fs_i_type));
        std::size_t file_size = m_input_file->size();


        // compute the number of blocks in the file
        int total_blocks = static_cast<int>(file_size / block_size);

        if(file_size % block_size != 0) {
            ++total_blocks;
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

        m_input_file = std::make_unique<posix_file::file>(
                posix_file::open(m_input_path, O_RDONLY, 0, m_fs_i_type));

        m_workers_size = workers_size;
        m_workers_rank = workers_rank;
        m_block_size = block_size;
        m_file_size = file_size;
        m_total_blocks = total_blocks;

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

    return error_code::transfer_in_progress;
}

cargo::error_code
seq_operation::progress() const {
    return m_status;
}

int
seq_operation::progress(int ongoing_index) {
    using posix_file::views::all_of;
    using posix_file::views::as_blocks;
    using posix_file::views::strided;

    // compute the number of blocks in the file

    int index = 0;
    if(write == false) {
        if(ongoing_index == 0) {
            m_bytes_per_rank = 0;
        }
        try {
            for(const auto& file_range :
                all_of(*m_input_file) | as_blocks(m_block_size) |
                        strided(m_workers_size, m_workers_rank)) {

                if(index < ongoing_index) {
                    ++index;
                    continue;
                } else {
                    if(index > ongoing_index) {
                        return index;
                    }
                }
                m_status = error_code::transfer_in_progress;
                assert(m_buffer_regions[index].size() >= file_range.size());
                auto start = std::chrono::steady_clock::now();
                const std::size_t n = m_input_file->pread(
                        m_buffer_regions[index], file_range.offset(),
                        file_range.size());

                LOGGER_DEBUG("Buffer contents: [\"{}\" ... \"{}\"]",
                             fmt::join(buffer_regions[index].begin(),
                                       buffer_regions[index].begin() + 10, ""),
                             fmt::join(buffer_regions[index].end() - 10,
                                       buffer_regions[index].end(), ""));


                m_bytes_per_rank += n;
                // Do sleep
                std::this_thread::sleep_for(sleep_value());
                auto end = std::chrono::steady_clock::now();
                // Send transfer bw
                double elapsed_seconds =
                        std::chrono::duration_cast<
                                std::chrono::duration<double>>(end - start)
                                .count();
                if((elapsed_seconds) > 0) {
                    bw((m_block_size / (1024.0 * 1024.0)) / (elapsed_seconds));
                    LOGGER_DEBUG(
                            "BW (read) Update: {} / {} = {} mb/s [ Sleep {} ]",
                            m_block_size / 1024.0, elapsed_seconds, bw(),
                            sleep_value());
                }

                ++index;
            }
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
    }
    write = true;
    // We finished reading
    // step3. POSIX write data
    // We need to create the directory if it does not exists (using
    // FSPlugin)

    if(write and ongoing_index == 0) {
        m_output_file = std::make_unique<posix_file::file>(posix_file::create(
                m_output_path, O_WRONLY, S_IRUSR | S_IWUSR, m_fs_o_type));

        m_output_file->fallocate(0, 0, m_file_size);
    }

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
            // Do sleep
            std::this_thread::sleep_for(sleep_value());
            auto end = std::chrono::steady_clock::now();
            // Send transfer bw
            double elapsed_seconds =
                    std::chrono::duration_cast<std::chrono::duration<double>>(
                            end - start)
                            .count();
            if((elapsed_seconds) > 0) {
                bw((m_block_size / (1024.0 * 1024.0)) / (elapsed_seconds));
                LOGGER_DEBUG(
                        "BW (write) Update: {} / {} = {} mb/s [ Sleep {} ]",
                        m_block_size / 1024.0, elapsed_seconds, bw(),
                        sleep_value());
            }

            ++index;
        }

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

} // namespace cargo
