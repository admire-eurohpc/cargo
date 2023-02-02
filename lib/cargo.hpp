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


#ifndef CARGO_HPP
#define CARGO_HPP

#include <string>
#include <vector>

namespace cargo {

using transfer_id = std::uint64_t;
using error_code = std::int32_t;

/**
 * A Cargo server
 */
struct server {

    explicit server(std::string address) noexcept;

    [[nodiscard]] std::string
    protocol() const noexcept;

    [[nodiscard]] std::string
    address() const noexcept;

private:
    std::string m_protocol;
    std::string m_address;
};


/**
 * A dataset
 */
class dataset {

public:
    dataset() noexcept = default;
    explicit dataset(std::string id) noexcept;

    [[nodiscard]] std::string
    id() const noexcept;

    template <typename Archive>
    void
    serialize(Archive& ar) {
        ar& m_id;
    }

private:
    std::string m_id;
};


/**
 * A transfer handler
 */
class transfer {

public:
    transfer() noexcept = default;
    explicit transfer(transfer_id id) noexcept;

    [[nodiscard]] transfer_id
    id() const noexcept;

    template <typename Archive>
    void
    serialize(Archive& ar) {
        ar& m_id;
    }

private:
    transfer_id m_id;
};


/**
 * Request the transfer of a dataset collection.
 *
 * @param srv The Cargo server that should execute the transfer.
 * @param sources The input datasets that should be transferred.
 * @param targets The output datasets that should be generated.
 * @return A transfer
 */
cargo::transfer
transfer_datasets(const server& srv, const std::vector<dataset>& sources,
                  const std::vector<dataset>& targets);

} // namespace cargo

#endif // CARGO_HPP
