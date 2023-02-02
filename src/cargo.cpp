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
#include <boost/program_options.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <exception>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <version.hpp>
#include <config/settings.hpp>
#include <boost/mpi.hpp>
#include "master.hpp"
#include "worker.hpp"
#include "env.hpp"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
namespace mpi = boost::mpi;

// utility functions
namespace {

void
print_version(const std::string& progname) {
    fmt::print("{} {}\n", progname, cargo::version_string);
}

void
print_help(const std::string& progname,
           const bpo::options_description& opt_desc) {
    fmt::print("Usage: {} [options]\n\n", progname);
    fmt::print("{}", opt_desc);
}

std::unordered_map<std::string, std::string>
load_environment_variables() {

    std::unordered_map<std::string, std::string> envs;

    if(const auto p = std::getenv(cargo::env::LOG);
       p && !std::string{p}.empty() && std::string{p} != "0") {

        if(const auto log_file = std::getenv(cargo::env::LOG_OUTPUT)) {
            envs.emplace(cargo::env::LOG_OUTPUT, log_file);
        }
    }

    return envs;
}

bpo::options_description
setup_command_line_args(config::settings& cfg) {


    // define the command line options allowed
    bpo::options_description opt_desc("Options");
    opt_desc.add_options()
            // force logging messages to the console
            ("force-console,C",
             bpo::value<std::string>()
                     ->implicit_value("")
                     ->zero_tokens()
                     ->notifier([&](const std::string&) {
                         cfg.log_file(fs::path{});
                         cfg.use_console(true);
                     }),
             "override any logging options defined in configuration files and "
             "send all daemon output to the console")

            // print the daemon version
            ("version,v",
             bpo::value<std::string>()->implicit_value("")->zero_tokens(),
             "print version string")

            // print help
            ("help,h",
             bpo::value<std::string>()->implicit_value("")->zero_tokens(),
             "produce help message");

    return opt_desc;
}

config::settings
parse_command_line(int argc, char* argv[]) {

    config::settings cfg;
    cfg.daemonize(false);

    const auto opt_desc = ::setup_command_line_args(cfg);

    // parse the command line
    bpo::variables_map vm;

    try {
        bpo::store(bpo::parse_command_line(argc, argv, opt_desc), vm);

        // the --help and --version arguments are special, since we want
        // to process them even if the global configuration file doesn't exist
        if(vm.count("help")) {
            print_help(cfg.progname(), opt_desc);
            exit(EXIT_SUCCESS);
        }

        if(vm.count("version")) {
            print_version(cfg.progname());
            exit(EXIT_SUCCESS);
        }

        const fs::path config_file = (vm.count("config-file") == 0)
                                             ? cfg.config_file()
                                             : vm["config-file"].as<fs::path>();

        if(!fs::exists(config_file)) {
            fmt::print(stderr,
                       "Failed to access daemon configuration file {}\n",
                       config_file);
            exit(EXIT_FAILURE);
        }

        try {
            cfg.load_from_file(config_file);
        } catch(const std::exception& ex) {
            fmt::print(stderr,
                       "Failed reading daemon configuration file:\n"
                       "    {}\n",
                       ex.what());
            exit(EXIT_FAILURE);
        }

        // override settings from the configuration file with settings
        // from environment variables
        const auto env_opts = load_environment_variables();

        if(const auto& it = env_opts.find(cargo::env::LOG_OUTPUT);
           it != env_opts.end()) {
            cfg.log_file(it->second);
        }

        // calling notify() here basically invokes all define notifiers, thus
        // overriding any configuration loaded from the global configuration
        // file with its command-line counterparts if provided (for those
        // options where this is available)
        bpo::notify(vm);

        return cfg;
    } catch(const bpo::error& ex) {
        fmt::print(stderr, "ERROR: {}\n\n", ex.what());
        exit(EXIT_FAILURE);
    }
}

} // namespace

int
main(int argc, char* argv[]) {

    config::settings cfg = parse_command_line(argc, argv);

    // Initialize the MPI environment
    mpi::environment env;
    mpi::communicator world;

    try {
        if(world.rank() == 0) {
            master(cfg);
        } else {
            worker();
        }
    } catch(const std::exception& ex) {
        fmt::print(stderr,
                   "An unhandled exception reached the top of main(), "
                   "{} will exit:\n  what():  {}\n",
                   cfg.progname(), ex.what());

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
