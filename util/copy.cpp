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
#include <cargo.hpp>
#include <fmt_formatters.hpp>
#include <filesystem>
#include <CLI/CLI.hpp>
#include <ranges>

enum class dataset_flags { posix, mpio };

std::map<std::string, dataset_flags> dataset_flags_map{
        {"posix", dataset_flags::posix},
        {"mpio", dataset_flags::mpio}};

struct copy_config {
    std::string progname;
    std::string server_address;
    std::vector<std::filesystem::path> inputs;
    dataset_flags input_flags = dataset_flags::posix;
    std::vector<std::filesystem::path> outputs;
    dataset_flags output_flags = dataset_flags::posix;
};

copy_config
parse_command_line(int argc, char* argv[]) {

    copy_config cfg;

    cfg.progname = std::filesystem::path{argv[0]}.filename().string();

    CLI::App app{"Cargo parallel copy tool", cfg.progname};

    app.add_option("-s,--server", cfg.server_address,
                   "Address of the Cargo server (can also be\n"
                   "provided via the CCP_SERVER environment\n"
                   "variable)")
            ->option_text("ADDRESS")
            ->envname("CCP_SERVER")
            ->required();

    app.add_option("-i,--input", cfg.inputs, "Input dataset(s)")
            ->option_text("SRC...")
            ->required();

    app.add_option("-o,--output", cfg.outputs, "Output dataset(s)")
            ->option_text("DST...")
            ->required();

    app.add_option("--if", cfg.input_flags,
                   "Flags for input datasets. Accepted values\n"
                   "  - posix: read data using POSIX (default)\n"
                   "  - mpio: read data using MPI-IO")
            ->option_text("FLAGS")
            ->transform(CLI::CheckedTransformer(dataset_flags_map,
                                                CLI::ignore_case));

    app.add_option("--of", cfg.output_flags,
                   "Flags for output datasets. Accepted values\n"
                   "  - posix: write data using POSIX (default)\n"
                   "  - mpio: write data using MPI-IO")
            ->option_text("FLAGS")
            ->transform(CLI::CheckedTransformer(dataset_flags_map,
                                                CLI::ignore_case));

    try {
        app.parse(argc, argv);
        return cfg;
    } catch(const CLI::ParseError& ex) {
        std::exit(app.exit(ex));
    }
}

auto
parse_address(const std::string& address) {
    const auto pos = address.find("://");
    if(pos == std::string::npos) {
        throw std::runtime_error(fmt::format("Invalid address: {}", address));
    }

    const auto protocol = address.substr(0, pos);
    return std::make_pair(protocol, address);
}

int
main(int argc, char* argv[]) {

    const auto cfg = parse_command_line(argc, argv);

    try {
        const auto [protocol, address] = parse_address(cfg.server_address);

        cargo::server server{address};
        std::vector<cargo::dataset> inputs;
        std::vector<cargo::dataset> outputs;

        std::transform(cfg.inputs.cbegin(), cfg.inputs.cend(),
                       std::back_inserter(inputs), [&](const auto& src) {
                           return cargo::dataset{
                                   src, cfg.input_flags == dataset_flags::mpio
                                                ? cargo::dataset::type::parallel
                                                : cargo::dataset::type::posix};
                       });

        std::transform(cfg.outputs.cbegin(), cfg.outputs.cend(),
                       std::back_inserter(outputs), [&cfg](const auto& tgt) {
                           return cargo::dataset{
                                   tgt, cfg.output_flags == dataset_flags::mpio
                                                ? cargo::dataset::type::parallel
                                                : cargo::dataset::type::posix};
                       });

        const auto tx = cargo::transfer_datasets(server, inputs, outputs);

        if(const auto st = tx.wait(); st.failed()) {
            throw std::runtime_error(st.error().message());
        }
    } catch(const std::exception& ex) {
        fmt::print(stderr, "{}: Error: {}\n", cfg.progname, ex.what());
        return EXIT_FAILURE;
    }
}
