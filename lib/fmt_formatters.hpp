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

#ifndef CARGO_FMT_FORMATTERS_HPP
#define CARGO_FMT_FORMATTERS_HPP

#include <iomanip>
#include <vector>
#include <string_view>
#include <optional>
#include <fmt/format.h>
#include "cargo/error.hpp"

namespace cargo {

class dataset;

} // namespace cargo

template <>
struct fmt::formatter<cargo::dataset> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::dataset& d, FormatContext& ctx) const {
        const auto str = fmt::format("{{path: {}}}", std::quoted(d.path()));
        return formatter<std::string_view>::format(str, ctx);
    }
};

template <>
struct fmt::formatter<std::vector<cargo::dataset>>
    : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const std::vector<cargo::dataset>& v, FormatContext& ctx) const {
        const auto str = fmt::format("[{}]", fmt::join(v, ", "));
        return formatter<std::string_view>::format(str, ctx);
    }
};

template <>
struct fmt::formatter<cargo::transfer> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::transfer& tx, FormatContext& ctx) const {
        const auto str = fmt::format("{{tid: {}}}", tx.id());
        return formatter<std::string_view>::format(str, ctx);
    }
};

template <>
struct fmt::formatter<cargo::error_code> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::error_code& ec, FormatContext& ctx) const {
        return formatter<std::string_view>::format(ec.name(), ctx);
    }
};

template <>
struct fmt::formatter<cargo::transfer_state> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::transfer_state& s, FormatContext& ctx) const {
        switch(s) {
            case cargo::transfer_state::pending:
                return formatter<std::string_view>::format("pending", ctx);
            case cargo::transfer_state::running:
                return formatter<std::string_view>::format("running", ctx);
            case cargo::transfer_state::completed:
                return formatter<std::string_view>::format("completed", ctx);
            case cargo::transfer_state::failed:
                return formatter<std::string_view>::format("failed", ctx);
            default:
                return formatter<std::string_view>::format("unknown", ctx);
        }
    }
};

template <typename T>
struct fmt::formatter<std::optional<T>> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const std::optional<T>& v, FormatContext& ctx) const {
        return formatter<std::string_view>::format(
                v ? fmt::format("{}", v.value()) : "none", ctx);
    }
};


#endif // CARGO_FMT_FORMATTERS_HPP
