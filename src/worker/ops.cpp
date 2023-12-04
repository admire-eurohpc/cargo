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
                          std::filesystem::path output_path,
                          std::uint64_t block_size, FSPlugin::type fs_i_type,
                          FSPlugin::type fs_o_type) {
    using cargo::tag;
    switch(t) {
        case tag::pread:
            return std::make_unique<mpio_read>(
                    std::move(workers), std::move(input_path),
                    std::move(output_path), block_size, fs_i_type, fs_o_type);
        case tag::pwrite:
            return std::make_unique<mpio_write>(
                    std::move(workers), std::move(input_path),
                    std::move(output_path), block_size, fs_i_type, fs_o_type);
        case tag::sequential:
            return std::make_unique<seq_operation>(
                    std::move(workers), std::move(input_path),
                    std::move(output_path), block_size, fs_i_type, fs_o_type);
        default:
            return {};
    }
}

std::chrono::milliseconds
operation::sleep_value() const {
    if(m_sleep_value <= 0)
        return std::chrono::milliseconds{0};
    else
        return std::chrono::milliseconds{m_sleep_value * 100};
}

void
operation::set_bw_shaping(std::int16_t incr) {
    m_sleep_value += incr;
}

int
operation::source() {
    return m_rank;
}
std::uint64_t
operation::tid() {
    return m_tid;
}
std::uint32_t
operation::seqno() {
    return m_seqno;
}

cargo::tag
operation::t() {
    return m_t;
}

float_t
operation::bw() {
    return m_bw;
}

void
operation::bw(float_t bw) {
    m_bw = bw;
}
void
operation::set_comm(int rank, std::uint64_t tid, std::uint32_t seqno,
                    cargo::tag t) {
    m_rank = rank;
    m_tid = tid;
    m_seqno = seqno;
    m_t = t;
}

cargo::error_code
operation::progress() const {
    return error_code::other;
}

} // namespace cargo
