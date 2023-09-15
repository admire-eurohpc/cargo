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

#include <fmt/format.h>
#include <logger/logger.hpp>
#include <boost/mpi.hpp>
#include <boost/mpi/error_string.hpp>
#include <span>
#include <thread>
#include <posix_file/file.hpp>
#include <posix_file/views.hpp>
#include "worker.hpp"
#include "proto/mpi/message.hpp"
#include "mpioxx.hpp"

namespace mpi = boost::mpi;
using namespace std::chrono_literals;


namespace {

// boost MPI doesn't have a communicator constructor that uses
// MPI_Comm_create_group()
mpi::communicator
make_communicator(const mpi::communicator& comm, const mpi::group& group,
                  int tag) {
    MPI_Comm newcomm;
    if(const auto ec = MPI_Comm_create_group(comm, group, tag, &newcomm);
       ec != MPI_SUCCESS) {
        LOGGER_ERROR("MPI_Comm_create_group() failed: {}",
                     boost::mpi::error_string(ec));
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    return mpi::communicator{newcomm, boost::mpi::comm_take_ownership};
}


} // namespace

namespace cargo {

using memory_buffer = std::vector<char>;
using buffer_region = std::span<char>;

void
mpio_read(const mpi::communicator& workers,
          const std::filesystem::path& input_path,
          const std::filesystem::path& output_path) {

    using posix_file::views::all_of;
    using posix_file::views::as_blocks;
    using posix_file::views::strided;

    const auto input_file = mpioxx::file::open(workers, input_path,
                                               mpioxx::file_open_mode::rdonly);

    mpioxx::offset file_size = input_file.size();
    std::size_t block_size = 512u;

    // create block type
    MPI_Datatype block_type;
    MPI_Type_contiguous(static_cast<int>(block_size), MPI_BYTE, &block_type);
    MPI_Type_commit(&block_type);

    // compute the number of blocks in the file
    int total_blocks = static_cast<int>(file_size / block_size);

    if(file_size % block_size != 0) {
        ++total_blocks;
    }

    const auto workers_size = workers.size();
    const auto workers_rank = workers.rank();

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
        LOGGER_ERROR("MPI_File_set_view() failed: {}", mpi::error_string(ec));
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // find how many blocks this rank is responsible for
    std::size_t blocks_per_rank = total_blocks / workers_size;

    if(int64_t n = total_blocks % workers_size; n != 0 && workers_rank < n) {
        ++blocks_per_rank;
    }

    // step 1. acquire buffers
    memory_buffer buffer;
    buffer.resize(blocks_per_rank * block_size);

    std::vector<buffer_region> buffer_regions;
    buffer_regions.reserve(blocks_per_rank);

    for(std::size_t i = 0; i < blocks_per_rank; ++i) {
        buffer_regions.emplace_back(buffer.data() + i * block_size, block_size);
    }

    MPI_Datatype datatype = block_type;

    // step2. parallel read data into buffers
    if(const auto ec = MPI_File_read_all(input_file, buffer.data(),
                                         static_cast<int>(blocks_per_rank),
                                         datatype, MPI_STATUS_IGNORE);
            ec != MPI_SUCCESS) {
        LOGGER_ERROR("MPI_File_read_all() failed: {}", mpi::error_string(ec));
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // step3. POSIX write data
    if(const auto rv =
                posix_file::create(output_path, O_WRONLY, S_IRUSR | S_IWUSR);
            !rv) {
        LOGGER_ERROR("posix_file::create({}) failed: {}", output_path,
                     rv.error().message());
    } else {

        const auto& output_file = rv.value();

        if(const auto ret = output_file.fallocate(0, 0, file_size); !rv) {
            LOGGER_ERROR("posix_file::fallocate({}, {}, {}) failed: {}", 0, 0,
                         file_size, ret.error().message());
            // TODO  : gracefully fail
        }

        int index = 0;
        for(const auto& file_range :
                all_of(posix_file::file{input_path}) | as_blocks(block_size) |
                strided(workers_size, workers_rank)) {
            assert(buffer_regions[index].size() >= file_range.size());
            const auto ret =
                    output_file.pwrite(buffer_regions[index],
                                       file_range.offset(), file_range.size());

            if(!ret) {
                LOGGER_ERROR("pwrite() failed: {}", ret.error().message());
            }

            ++index;
        }
    }
}

void
mpio_write(const mpi::communicator& workers,
           const std::filesystem::path& input_path,
           const std::filesystem::path& output_path) {

    using posix_file::views::all_of;
    using posix_file::views::as_blocks;
    using posix_file::views::strided;

    const auto workers_size = workers.size();
    const auto workers_rank = workers.rank();
    std::size_t block_size = 512u;
    std::size_t file_size = std::filesystem::file_size(input_path);

    // compute the number of blocks in the file
    int total_blocks = static_cast<int>(file_size / block_size);

    if(file_size % block_size != 0) {
        ++total_blocks;
    }

    // find how many blocks this rank is responsible for
    std::size_t blocks_per_rank = total_blocks / workers_size;

    if(int64_t n = total_blocks % workers_size; n != 0 && workers_rank < n) {
        ++blocks_per_rank;
    }

    // step 1. acquire buffers
    memory_buffer buffer;
    buffer.resize(blocks_per_rank * block_size);

    std::vector<buffer_region> buffer_regions;
    buffer_regions.reserve(blocks_per_rank);

    for(std::size_t i = 0; i < blocks_per_rank; ++i) {
        buffer_regions.emplace_back(buffer.data() + i * block_size, block_size);
    }

    const auto rv = posix_file::open(input_path, O_RDONLY);

    if(!rv) {
        LOGGER_ERROR("posix_file::open({}) failed: {} ", input_path,
                     rv.error().message());
        // TODO  : gracefully fail
    }

    int index = 0;
    std::size_t bytes_per_rank = 0;

    for(const auto& input_file = rv.value();
            const auto& file_range : all_of(input_file) | as_blocks(block_size) |
                                     strided(workers_size, workers_rank)) {

        assert(buffer_regions[index].size() >= file_range.size());
        const auto ret = input_file.pread(
                buffer_regions[index], file_range.offset(), file_range.size());

        if(!ret) {
            LOGGER_ERROR("pread() failed: {}", ret.error().message());
        }

        LOGGER_DEBUG("Buffer contents: [\"{}\" ... \"{}\"]",
                     fmt::join(buffer_regions[index].begin(),
                               buffer_regions[index].begin() + 10, ""),
                     fmt::join(buffer_regions[index].end() - 10,
                               buffer_regions[index].end(), ""));

        bytes_per_rank += ret.value();
        ++index;
    }

    // step 2. write buffer data in parallel to the PFS
    const auto output_file = mpioxx::file::open(
            workers, output_path,
            mpioxx::file_open_mode::create | mpioxx::file_open_mode::wronly);

    // create block type
    MPI_Datatype block_type;
    MPI_Type_contiguous(static_cast<int>(block_size), MPI_BYTE, &block_type);
    MPI_Type_commit(&block_type);

    // create file type
    MPI_Datatype file_type;

    /*
     * count: number of blocks in the type
     * blocklen: number of `oldtype` elements in each block
     * stride: number of `oldtype` elements between start of each block
     */
    MPI_Type_vector(/* count: */ total_blocks, /* blocklength: */ 1,
            /* stride: */ workers_size, /* oldtype: */ block_type,
                                 &file_type);
    MPI_Type_commit(&file_type);

    if(const auto ec = MPI_File_set_view(output_file,
                /* disp: */ workers_rank * block_size,
                /* elementary_type: */ block_type,
                                         file_type, "native", MPI_INFO_NULL);
            ec != MPI_SUCCESS) {
        LOGGER_ERROR("MPI_File_set_view() failed: {}", mpi::error_string(ec));
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // step 3. parallel write data from buffers
    if(const auto ec = MPI_File_write_all(output_file, buffer.data(),
                                          static_cast<int>(bytes_per_rank),
                                          MPI_BYTE, MPI_STATUS_IGNORE);
            ec != MPI_SUCCESS) {
        LOGGER_ERROR("MPI_File_write_all() failed: {}", mpi::error_string(ec));
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
}

void
sequential_transfer(const std::filesystem::path& input_path,
                    const std::filesystem::path& output_path) {
    (void) input_path;
    (void) output_path;

    LOGGER_CRITICAL("{}: to be implemented", __FUNCTION__);
}

worker::worker(int rank) : m_rank(rank) {}

int
worker::run() {

    // Create a separate communicator only for worker processes
    const mpi::communicator world;
    const auto ranks_to_exclude = std::array<int, 1>{0};
    const auto workers =
            ::make_communicator(world,
                                world.group().exclude(ranks_to_exclude.begin(),
                                                      ranks_to_exclude.end()),
                                0);

    LOGGER_INIT(fmt::format("worker_{:03}", world.rank()),
                logger::console_color);

    // Initialization finished
    LOGGER_INFO("Staging process initialized (world_rank {}, workers_rank: {})",
                world.rank(), workers.rank());

    bool done = false;

    while(!done) {

        auto msg = world.iprobe();

        if(!msg) {
            // FIXME: sleep time should be configurable
            std::this_thread::sleep_for(150ms);
            continue;
        }

        switch(static_cast<tag>(msg->tag())) {
            case tag::transfer: {
                transfer_message m;
                world.recv(0, msg->tag(), m);
                LOGGER_DEBUG("Transfer request received!: {}", m);

                switch(m.type()) {
                    case parallel_read:
                        mpio_read(workers, m.input_path(), m.output_path());
                        break;
                    case parallel_write:
                        mpio_write(workers, m.input_path(), m.output_path());
                        break;
                    case sequential:
                        sequential_transfer(m.input_path(), m.output_path());
                        break;
                }

                LOGGER_CRITICAL(
                        "Transfer finished! (world_rank {}, workers_rank: {})",
                        world.rank(), workers.rank());

                world.send(msg->source(), static_cast<int>(tag::status),
                           status_message{m.tid(), m.seqno(),
                                          error_code::success});

                break;
            }

            default:
                LOGGER_WARN("[{}] Unexpected message tag: {}", msg->source(),
                            msg->tag());
                break;
        }
    }

    return 0;
}

} // namespace cargo
