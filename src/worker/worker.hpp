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


#ifndef CARGO_WORKER_HPP
#define CARGO_WORKER_HPP

namespace cargo {

class worker {
public:
    worker(std::string name, int rank);

    void
    set_output_file(std::filesystem::path output_file);

    int
    run();

private:
    std::string m_name;
    int m_rank;
    std::optional<std::filesystem::path> m_output_file;
};

} // namespace cargo

#endif // CARGO_WORKER_HPP
