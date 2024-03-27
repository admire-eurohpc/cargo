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
#include <filesystem>
#include <CLI/CLI.hpp>
#include <net/client.hpp>
#include <net/endpoint.hpp>

struct ftio_config {
    std::string progname;
    std::string server_address;
    float confidence;
    float probability;
    float period;
    bool run{false};
};

ftio_config
parse_command_line(int argc, char* argv[]) {

    ftio_config cfg;

    cfg.progname = std::filesystem::path{argv[0]}.filename().string();

    CLI::App app{"Cargo ftio client", cfg.progname};

    app.add_option("-s,--server", cfg.server_address, "Server address")
            ->option_text("ADDRESS")
            ->required();

    app.add_option("-c,--conf", cfg.confidence, "confidence")
            ->option_text("float")
            ->default_val("-1.0");

    app.add_option("-p,--probability", cfg.probability, "probability")
            ->option_text("float")
            ->default_val("-1.0");

    app.add_option("-t,--period", cfg.period, "period")
            ->option_text("float")
            ->default_val("-1.0");
    app.add_flag(
            "--run", cfg.run,
            "Trigger stage operation to run now. Has no effect when period is set > 0");

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

    ftio_config cfg = parse_command_line(argc, argv);

    try {
        const auto [protocol, address] = parse_address(cfg.server_address);
        network::client rpc_client{protocol};

        if(const auto result = rpc_client.lookup(address); result.has_value()) {
            const auto& endpoint = result.value();
            const auto retval =
                    endpoint.call("ftio_int", cfg.confidence, cfg.probability,
                                  cfg.period, cfg.run);

            if(retval.has_value()) {

                auto error_code = int{retval.value()};

                fmt::print("ftio_int RPC was successful!\n");
                fmt::print("  (server replied with: {})\n", error_code);
                return EXIT_SUCCESS;
            }

            fmt::print(stderr, "ftio_int RPC failed\n");
            return EXIT_FAILURE;

        } else {
            fmt::print(stderr, "Failed to lookup address: {}\n", address);
            return EXIT_FAILURE;
        }
    } catch(const std::exception& ex) {
        fmt::print(stderr, "Error: {}\n", ex.what());
        return EXIT_FAILURE;
    }
}
