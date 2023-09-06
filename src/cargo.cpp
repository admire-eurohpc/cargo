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


#include <filesystem>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <exception>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <CLI/CLI.hpp>
#include <boost/mpi.hpp>

#include <version.hpp>
#include "master.hpp"
#include "worker.hpp"
#include "env.hpp"

namespace fs = std::filesystem;
namespace mpi = boost::mpi;

// utility functions
namespace {

struct cargo_config {
    std::string progname;
    bool daemonize = false;
    std::optional<fs::path> output_file;
    std::string address;
};

cargo_config
parse_command_line(int argc, char* argv[]) {

    cargo_config cfg;

    cfg.progname = fs::path{argv[0]}.filename().string();

    CLI::App app{"Cargo: A parallel data staging framework for HPC",
                 cfg.progname};

    // force logging messages to file
    app.add_option("-o,--output", cfg.output_file,
                   "Write any output to FILENAME rather than sending it to the "
                   "console")
            ->option_text("FILENAME");

    app.add_option("-l,--listen", cfg.address,
                   "Address or interface to bind the daemon to. If using "
                   "`libfabric`,\n"
                   "the address is typically in the form of:\n\n"
                   "  ofi+<protocol>[://<hostname,IP,interface>:<port>]\n\n"
                   "Check `fi_info` to see the list of available protocols.\n")
            ->option_text("ADDRESS")
            ->required();

    app.add_flag_function(
            "-v,--version",
            [&](auto /*count*/) {
                fmt::print("{} {}\n", cfg.progname, cargo::version_string);
                std::exit(EXIT_SUCCESS);
            },
            "Print version and exit");

    try {
        app.parse(argc, argv);
        return cfg;
    } catch(const CLI::ParseError& ex) {
        std::exit(app.exit(ex));
    }
}

} // namespace

int
main(int argc, char* argv[]) {

    cargo_config cfg = parse_command_line(argc, argv);

    // Initialize the MPI environment
    mpi::environment env;
    mpi::communicator world;

    try {
        if(world.rank() == 0) {

            cargo::master_server srv{cfg.progname, cfg.address, cfg.daemonize,
                                     fs::current_path()};

            if(cfg.output_file) {
                srv.configure_logger(logger::logger_type::file,
                                     *cfg.output_file);
            }

            return srv.run();

        } else {
            worker();
        }
    } catch(const std::exception& ex) {
        fmt::print(stderr,
                   "An unhandled exception reached the top of main(), "
                   "{} will exit:\n  what():  {}\n",
                   cfg.progname, ex.what());

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
