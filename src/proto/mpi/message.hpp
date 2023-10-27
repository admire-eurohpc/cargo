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

#ifndef CARGO_PROTO_MPI_MESSAGE_HPP
#define CARGO_PROTO_MPI_MESSAGE_HPP

#include <fmt/format.h>
#include <filesystem>
#include <boost/archive/binary_oarchive.hpp>
#include <utility>
#include <optional>
#include "cargo.hpp"
#include "boost_serialization_std_optional.hpp"

namespace cargo {

enum class tag : int {
    pread,
    pwrite,
    sequential,
    bw_shaping,
    status,
    shutdown
};

class transfer_message {

    friend class boost::serialization::access;

public:
    transfer_message() = default;

    transfer_message(std::uint64_t tid, std::uint32_t seqno,
                     std::string input_path, std::string output_path)
        : m_tid(tid), m_seqno(seqno), m_input_path(std::move(input_path)),
          m_output_path(std::move(output_path)) {}

    [[nodiscard]] std::uint64_t
    tid() const {
        return m_tid;
    }

    [[nodiscard]] std::uint32_t
    seqno() const {
        return m_seqno;
    }

    [[nodiscard]] const std::string&
    input_path() const {
        return m_input_path;
    }

    [[nodiscard]] const std::string&
    output_path() const {
        return m_output_path;
    }

private:
    template <class Archive>
    void
    serialize(Archive& ar, const unsigned int version) {
        (void) version;

        ar& m_tid;
        ar& m_seqno;
        ar& m_input_path;
        ar& m_output_path;
    }

    std::uint64_t m_tid{};
    std::uint32_t m_seqno{};
    std::string m_input_path;
    std::string m_output_path;
};

class status_message {

    friend class boost::serialization::access;

public:
    status_message() = default;

    status_message(std::uint64_t tid, std::uint32_t seqno,
                   cargo::transfer_state state, float bw,
                   std::optional<cargo::error_code> error_code = std::nullopt)
        : m_tid(tid), m_seqno(seqno), m_state(state), m_bw(bw),
          m_error_code(error_code) {}

    [[nodiscard]] std::uint64_t
    tid() const {
        return m_tid;
    }

    [[nodiscard]] std::uint32_t
    seqno() const {
        return m_seqno;
    }

    [[nodiscard]] cargo::transfer_state
    state() const {
        return m_state;
    }

    [[nodiscard]] float
    bw() const {
        return m_bw;
    }


    [[nodiscard]] std::optional<cargo::error_code>
    error_code() const {
        return m_error_code;
    }

private:
    template <class Archive>
    void
    serialize(Archive& ar, const unsigned int version) {
        (void) version;

        ar& m_tid;
        ar& m_seqno;
        ar& m_state;
        ar& m_bw;
        ar& m_error_code;
    }

    std::uint64_t m_tid{};
    std::uint32_t m_seqno{};
    cargo::transfer_state m_state{};
    float m_bw{};
    std::optional<cargo::error_code> m_error_code{};
};

class shaper_message {

    friend class boost::serialization::access;

public:
    shaper_message() = default;

    shaper_message(std::uint64_t tid, std::int16_t shaping)
        : m_tid(tid), m_shaping(shaping) {}

    [[nodiscard]] std::uint64_t
    tid() const {
        return m_tid;
    }

    [[nodiscard]] std::int16_t
    shaping() const {
        return m_shaping;
    }

private:
    template <class Archive>
    void
    serialize(Archive& ar, const unsigned int version) {
        (void) version;

        ar& m_tid;
        ar& m_shaping;
    }

    std::uint64_t m_tid{};
    std::uint16_t m_shaping{};
};


class shutdown_message {

    friend class boost::serialization::access;

public:
    shutdown_message() = default;

    template <typename Archive>
    void
    serialize(Archive& ar, const unsigned int version) {
        (void) ar;
        (void) version;
    }
};

} // namespace cargo

template <>
struct fmt::formatter<cargo::transfer_message> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::transfer_message& r, FormatContext& ctx) const {
        const auto str = fmt::format(
                "{{tid: {}, seqno: {}, input_path: {}, output_path: {}}}",
                r.tid(), r.seqno(), r.input_path(), r.output_path());
        return formatter<std::string_view>::format(str, ctx);
    }
};

template <>
struct fmt::formatter<cargo::status_message> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::status_message& s, FormatContext& ctx) const {
        const auto str =
                s.error_code()
                        ? fmt::format(
                                  "{{tid: {}, seqno: {}, state: {}, bw: {}, "
                                  "error_code: {}}}",
                                  s.tid(), s.seqno(), s.state(), s.bw(),
                                  *s.error_code())
                        : fmt::format(
                                  "{{tid: {}, seqno: {}, state: {}, bw: {}}}",
                                  s.tid(), s.seqno(), s.state(), s.bw());
        return formatter<std::string_view>::format(str, ctx);
    }
};

template <>
struct fmt::formatter<cargo::shaper_message> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::shaper_message& s, FormatContext& ctx) const {
        const auto str =
                fmt::format("{{tid: {}, shaping: {}}}", s.tid(), s.shaping());
        return formatter<std::string_view>::format(str, ctx);
    }
};

template <>
struct fmt::formatter<cargo::shaper_message> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::shaper_message& s, FormatContext& ctx) const {
    const auto str = fmt::format("{{tid: {}, shaping: {}}}", s.tid(), s.shaping());
        return formatter<std::string_view>::format(str, ctx);
    }
};

template <>
struct fmt::formatter<cargo::shutdown_message> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::shutdown_message& s, FormatContext& ctx) const {
        (void) s;
        return formatter<std::string_view>::format("{{shutdown}}", ctx);
    }
};

#endif // CARGO_PROTO_MPI_MESSAGE_HPP
