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
#include <string_view>
#include <fmt/format.h>

namespace cargo {

class dataset;

} // namespace cargo

template <>
struct fmt::formatter<cargo::dataset> : formatter<std::string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto
    format(const cargo::dataset& d, FormatContext& ctx) const {
        const auto str = fmt::format("{{id: {}}}", std::quoted(d.id()));
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
        const auto str = fmt::format("{{id: {}}}", tx.id());
        return formatter<std::string_view>::format(str, ctx);
    }
};

#endif // CARGO_FMT_FORMATTERS_HPP
