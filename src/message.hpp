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

#ifndef CARGO_MESSAGE_HPP
#define CARGO_MESSAGE_HPP

#include <fmt/format.h>
#include <filesystem>
#include <boost/archive/binary_oarchive.hpp>
#include <utility>

namespace cargo {

enum transfer_type { parallel_read, parallel_write, sequential };
enum class message_tags { transfer, status, shutdown };

class transfer_request_message {

    friend class boost::serialization::access;

public:
    transfer_request_message() = default;

    transfer_request_message(const std::filesystem::path& input_path,
                             const std::filesystem::path& output_path,
                             transfer_type type)
        : m_input_path(input_path), m_output_path(output_path), m_type(type) {}

    std::filesystem::path
    input_path() const {
        return m_input_path;
    }

    std::filesystem::path
    output_path() const {
        return m_output_path;
    }

    transfer_type
    type() const {
        return m_type;
    }

private:
    template <class Archive>
    void
    serialize(Archive& ar, const unsigned int version) {
        (void) version;

        ar& m_input_path;
        ar& m_output_path;
        ar& m_type;
    }

    std::string m_input_path;
    std::string m_output_path;
    transfer_type m_type;
};

class transfer_status_message {

    friend class boost::serialization::access;

public:
    transfer_status_message() = default;

    explicit transfer_status_message(std::uint64_t transfer_id)
        : m_transfer_id(transfer_id) {}

    std::uint64_t
    transfer_id() const {
        return m_transfer_id;
    }

private:
    template <class Archive>
    void
    serialize(Archive& ar, const unsigned int version) {
        (void) version;

        ar& m_transfer_id;
    }

    std::uint64_t m_transfer_id{};
};

} // namespace cargo

template <>
struct fmt::formatter<cargo::transfer_request_message>
    : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::transfer_request_message& r, FormatContext& ctx) const {
        const auto str = fmt::format("{{input_path: {}, output_path: {}}}",
                                     r.input_path(), r.output_path());
        return formatter<std::string_view>::format(str, ctx);
    }
};

template <>
struct fmt::formatter<cargo::transfer_status_message>
    : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::transfer_status_message& s, FormatContext& ctx) const {
        const auto str = fmt::format("{{transfer_id: {}}}", s.transfer_id());
        return formatter<std::string_view>::format(str, ctx);
    }
};

#endif // CARGO_MESSAGE_HPP
