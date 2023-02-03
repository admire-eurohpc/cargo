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
#include <fmt/chrono.h>
#include <thread>
#include <chrono>
#include <string_view>
#include <fstream>
#include <random>
#include <filesystem>
#include <cargo.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <catch2/catch_config.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include "common.hpp"

using namespace std::literals;
using namespace std::chrono_literals;

struct scoped_file {
    explicit scoped_file(std::filesystem::path filepath)
        : m_filepath(std::move(filepath)) {}

    scoped_file(scoped_file&& rhs) noexcept
        : m_filepath(std::move(rhs.m_filepath)) {}

    scoped_file(scoped_file& other) = delete;

    scoped_file&
    operator=(scoped_file&& rhs) noexcept {
        if(this != &rhs) {
            m_filepath = std::move(rhs.m_filepath);
        }
        return *this;
    };

    scoped_file&
    operator=(scoped_file& other) = delete;

    ~scoped_file() {
        if(!m_filepath.empty()) {
            std::filesystem::remove(m_filepath);
        }
    }

    auto
    path() const {
        return m_filepath;
    }

    std::filesystem::path m_filepath;
};

struct ascii_data_generator {

    static constexpr std::array<char, 26> letters = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
            'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
    using result_type = char;

    explicit ascii_data_generator(std::size_t block_size)
        : m_block_size(block_size) {}

    char
    operator()() const noexcept {

        const auto rv = *m_it;

        if((++m_index % m_block_size) == 0) {
            if(++m_it == letters.end()) {
                m_it = letters.begin();
            }
        }

        return rv;
    }

    mutable std::size_t m_index = 0;
    mutable decltype(letters)::const_iterator m_it = letters.begin();
    std::size_t m_block_size;
};

struct random_data_generator {

    using result_type = std::mt19937::result_type;

    explicit random_data_generator(std::size_t seed, std::size_t min,
                                   std::size_t max)
        : m_seed(seed), m_rng(seed), m_dist(min, max) {}

    auto
    operator()() const noexcept {
        return m_dist(m_rng);
    }

    constexpr auto
    seed() const noexcept {
        return m_seed;
    }

    std::size_t m_seed;
    std::random_device m_device;
    mutable std::mt19937 m_rng;
    mutable std::uniform_int_distribution<std::mt19937::result_type> m_dist;
};

template <typename DataGenerator>
scoped_file
create_temporary_file(const std::filesystem::path& name,
                      std::size_t desired_size,
                      const DataGenerator& generator) {

    char tmpname[] = "posix_file_tests_XXXXXX";
    file_handle fh{mkstemp(tmpname)};

    if(!fh) {
        fmt::print(stderr, "mkstemp() error: {}\n", strerror(errno));
        abort();
    }

    if(ftruncate(fh.native(), static_cast<off_t>(desired_size)) != 0) {
        fmt::print(stderr, "ftruncate() error: {}\n", strerror(errno));
        abort();
    }

    using result_type = DataGenerator::result_type;
    assert(desired_size % sizeof(result_type) == 0);
    const std::size_t n = desired_size / sizeof(result_type);
    std::vector<result_type> data(n);
    std::generate(data.begin(), data.end(), std::ref(generator));

    std::fstream f{tmpname, std::ios::out | std::ios::binary};
    f.write(reinterpret_cast<char*>(data.data()), desired_size);
    f.close();

    std::filesystem::rename(tmpname, name);

    REQUIRE(std::filesystem::exists(name));
    REQUIRE(std::filesystem::file_size(name) == desired_size);

    return scoped_file{name};
}

bool
equal(const std::filesystem::path& filepath1,
      const std::filesystem::path& filepath2) {
    namespace io = boost::iostreams;

    io::mapped_file_source f1(filepath1);
    io::mapped_file_source f2(filepath2);

    const auto start_time = std::chrono::steady_clock::now();

    bool rv = f1.size() == f2.size() &&
              std::equal(f1.data(), f1.data() + f1.size(), f2.data());

    const auto end_time = std::chrono::steady_clock::now();

    const auto duration =
            std::chrono::duration<double, std::micro>(end_time - start_time);

    fmt::print(stderr, "::equal(\"{}\", \"{}\"): {}\n", filepath1.string(),
               filepath2.string(), duration);

    return rv;
}


uint32_t catch2_seed;
std::string server_address;
#define NDATASETS 10

SCENARIO("Parallel reads", "[flex_stager][parallel_reads]") {

    random_data_generator rng{catch2_seed, 0,
                              std::numeric_limits<std::uint64_t>::max() - 1u};

    GIVEN("Some input datasets from a PFS") {

        REQUIRE(!server_address.empty());

        cargo::server server{server_address};

        const auto sources = prepare_datasets(cargo::dataset::type::parallel,
                                              "source-dataset-{}", NDATASETS);
        const auto targets = prepare_datasets(cargo::dataset::type::posix,
                                              "target-dataset-{}", NDATASETS);

        static std::vector<scoped_file> input_files;
        input_files.reserve(sources.size());

        for(const auto& d : sources) {
            input_files.emplace_back(
                    create_temporary_file(d.path(), 1000, rng));
        }

        // ensure there are no dangling output files from another test run
        std::for_each(targets.begin(), targets.end(), [](auto&& dataset) {
            std::filesystem::remove(dataset.path());
        });

        WHEN("Transferring datasets to a POSIX storage system") {
            const auto tx = cargo::transfer_datasets(server, sources, targets);

            (void) tx;

            // give time for transfers to complete before removing input files
            // FIXME: replace with proper status checking for the transfer
            std::this_thread::sleep_for(1s);

            THEN("Output datasets are identical to input datasets") {

                std::vector<scoped_file> output_files;
                std::transform(targets.cbegin(), targets.cend(),
                               std::back_inserter(output_files),
                               [](auto&& target) {
                                   return scoped_file{target.path()};
                               });

                for(std::size_t i = 0; i < input_files.size(); ++i) {
                    REQUIRE(std::filesystem::exists(output_files[i].path()));
                    REQUIRE(::equal(input_files[i].path(),
                                    output_files[i].path()));
                }
            }
        }
    }
}

SCENARIO("Parallel writes", "[flex_stager][parallel_writes]") {

    std::size_t file_size = 10000; // GENERATE(1000, 10000);

    [[maybe_unused]] ascii_data_generator ascii_gen{512};
    [[maybe_unused]] random_data_generator rng{
            catch2_seed, 0, std::numeric_limits<std::uint64_t>::max() - 1u};

    GIVEN("Some input datasets from a POSIX storage system") {

        REQUIRE(!server_address.empty());

        cargo::server server{server_address};

        const auto sources = prepare_datasets(cargo::dataset::type::posix,
                                              "source-dataset-{}", NDATASETS);
        const auto targets = prepare_datasets(cargo::dataset::type::parallel,
                                              "target-dataset-{}", NDATASETS);

        static std::vector<scoped_file> input_files;
        input_files.reserve(sources.size());

        for(const auto& d : sources) {
            input_files.emplace_back(
                    create_temporary_file(d.path(), file_size, rng));
        }

        // ensure there are no danling output files from another test run
        std::for_each(targets.begin(), targets.end(), [](auto&& dataset) {
            std::filesystem::remove(dataset.path());
        });

        WHEN("Transferring datasets to a PFS") {
            const auto tx = cargo::transfer_datasets(server, sources, targets);

            (void) tx;

            // give time for transfers to complete before removing input files
            // FIXME: replace with proper status checking for the transfer
            std::this_thread::sleep_for(1s);

            THEN("Output datasets are identical to input datasets") {

                std::vector<scoped_file> output_files;
                std::transform(targets.cbegin(), targets.cend(),
                               std::back_inserter(output_files),
                               [](auto&& target) {
                                   return scoped_file{target.path()};
                               });

                for(std::size_t i = 0; i < input_files.size(); ++i) {
                    REQUIRE(::equal(input_files[i].path(), targets[i].path()));
                }
            }
        }
    }
}

int
main(int argc, char* argv[]) {

    Catch::Session session; // There must be exactly one instance

    // Build a new parser on top of Catch2's
    using namespace Catch::Clara;
    auto cli =
            session.cli() // Get Catch2's command line parser
            | Opt(server_address,
                  "server_address")["-S"]["--server-address"]("server address");

    // Now pass the new composite back to Catch2, so it uses that
    session.cli(cli);

    // Let Catch2 (using Clara) parse the command line
    if(int returnCode = session.applyCommandLine(argc, argv); returnCode != 0) {
        return returnCode;
    }

    catch2_seed = session.config().rngSeed();

    return session.run();
}
