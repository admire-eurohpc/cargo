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

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <cargo/error.hpp>

namespace cargo {

using transfer_id = std::uint64_t;

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
    enum class type { posix, parallel };

    dataset() noexcept = default;

    explicit dataset(std::string path,
                     dataset::type type = dataset::type::posix) noexcept;

    [[nodiscard]] std::string
    path() const noexcept;

    [[nodiscard]] bool
    supports_parallel_transfer() const noexcept;

    template <typename Archive>
    void
    serialize(Archive& ar) {
        ar& m_path;
        ar& m_type;
    }

private:
    std::string m_path;
    dataset::type m_type = dataset::type::posix;
};


/**
 * The status of a Cargo transfer
 */
enum class transfer_state { pending, running, completed, failed };

class transfer_status;

/**
 * A transfer handler
 */
class transfer {

    friend transfer
    transfer_datasets(const server& srv, const std::vector<dataset>& sources,
                      const std::vector<dataset>& targets);

    explicit transfer(transfer_id id, server srv) noexcept;

    [[nodiscard]] transfer_id
    id() const noexcept;

public:
    /**
     * Get the current status of the associated transfer.
     *
     * @return A `transfer_status` object containing detailed information about
     * the transfer status.
     */
    [[nodiscard]] transfer_status
    status() const;

    /**
     * Wait for the associated transfer to complete.
     *
     * @return A `transfer_status` object containing detailed information about
     * the transfer status.
     */
    [[nodiscard]] transfer_status
    wait() const;

    /**
     * Wait for the associated transfer to complete or for a timeout to occur.
     * @param timeout The maximum amount of time to wait for the transfer to
     * complete.
     * @return A `transfer_status` object containing detailed information about
     * the transfer status.
     */
    [[nodiscard]] transfer_status
    wait_for(const std::chrono::nanoseconds& timeout) const;

private:
    transfer_id m_id;
    server m_srv;
};

/**
 * Detailed status information for a transfer
 */
class transfer_status {

    friend transfer_status
    transfer::status() const;

    transfer_status(transfer_state status, float bw, error_code error) noexcept;

public:
    /**
     * Get the current status of the associated transfer.
     *
     * @return A `transfer_state` enum value representing the current status.
     */
    [[nodiscard]] transfer_state
    state() const noexcept;

    /**
     * Check whether the transfer has completed.
     *
     * @return true if the transfer has completed, false otherwise.
     */
    [[nodiscard]] bool
    done() const noexcept;

    /**
     * Check whether the transfer has failed.
     *
     * @return true if the transfer has failed, false otherwise.
     */
    [[nodiscard]] bool
    failed() const noexcept;

    /**
     * Retrieve the error code associated with a failed transfer.
     *
     * @return An error code describing a transfer failure or
     * `error_code::success` if the transfer succeeded.
     * If the transfer has not yet completed,
     * `error_code::transfer_in_progress` is returned.
     */
    [[nodiscard]] error_code
    error() const;

    [[nodiscard]] float
    bw() const;

private:
    transfer_state m_state;
    float m_bw;
    error_code m_error;
};

/**
 * Request the transfer of a dataset collection.
 *
 * @param srv The Cargo server that should execute the transfer.
 * @param sources The input datasets that should be transferred.
 * @param targets The output datasets that should be generated.
 * @return A transfer
 */
transfer
transfer_datasets(const server& srv, const std::vector<dataset>& sources,
                  const std::vector<dataset>& targets);

/**
 * Request the transfer of a single dataset.
 * This function is a convenience wrapper around the previous one.
 * It takes a single source and a single target dataset.
 *
 * @param srv The Cargo server that should execute the transfer.
 * @param source The input dataset that should be transferred.
 * @param target The output dataset that should be generated.
 * @return A transfer
 */
transfer
transfer_dataset(const server& srv, const dataset& source,
                 const dataset& target);

} // namespace cargo

#endif // CARGO_HPP
