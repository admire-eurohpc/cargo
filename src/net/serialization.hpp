/******************************************************************************
 * Copyright 2023, Barcelona Supercomputing Center (BSC), Spain
 *
 * This software was partially supported by the EuroHPC-funded project ADMIRE
 *   (Project ID: 956748, https://www.admire-eurohpc.eu).
 *
 * This file is part of mochi-rpc-server-cxx.
 *
 * mochi-rpc-server-cxx is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mochi-rpc-server-cxx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mochi-rpc-server-cxx.  If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *****************************************************************************/

#ifndef NETWORK_SERIALIZATION_HPP
#define NETWORK_SERIALIZATION_HPP

#include <cereal/cereal.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/memory.hpp>
#include <thallium/serialization/proc_input_archive.hpp>
#include <thallium/serialization/proc_output_archive.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <thallium/serialization/stl/vector.hpp>

// Cereal does not serialize std::filesystem::path's by default
#include <filesystem>

namespace cereal {

//! Loading for std::filesystem::path
template <class Archive>
inline void
CEREAL_LOAD_FUNCTION_NAME(Archive& ar, std::filesystem::path& out) {
    std::string tmp;
    ar(CEREAL_NVP_("data", tmp));
    out.assign(tmp);
}

//! Saving for std::filesystem::path
template <class Archive>
inline void
CEREAL_SAVE_FUNCTION_NAME(Archive& ar, const std::filesystem::path& in) {
    ar(CEREAL_NVP_("data", in.string()));
}

} // namespace cereal

namespace network::serialization {

#define NETWORK_SERIALIZATION_NVP CEREAL_NVP

using input_archive = thallium::proc_input_archive<>;
using output_archive = thallium::proc_output_archive<>;

} // namespace network::serialization

#endif // NETWORK_SERIALIZATION_HPP
