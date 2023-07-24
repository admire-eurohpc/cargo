/******************************************************************************
 * Copyright 2021-2023, Barcelona Supercomputing Center (BSC), Spain
 *
 * This software was partially supported by the EuroHPC-funded project ADMIRE
 *   (Project ID: 956748, https://www.admire-eurohpc.eu).
 *
 * This file is part of scord.
 *
 * scord is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * scord is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with scord.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *****************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#define POSIX_FILE_HAVE_FMT
#include <posix_file/file.hpp>
#include <posix_file/views.hpp>
#include <algorithm>
#include <utility>
#include "catch2/generators/catch_generators_range.hpp"

using posix_file::views::all_of;
using posix_file::views::as_blocks;
using posix_file::views::some_of;
using posix_file::views::strided;

namespace Catch {
template <>
struct StringMaker<posix_file::ranges::range> {
    static std::string
    convert(const posix_file::ranges::range& r) {
        return fmt::format("{}", r);
    }
};
} // namespace Catch

struct scoped_file : public posix_file::file {

    explicit scoped_file(std::filesystem::path filepath)
        : posix_file::file(std::move(filepath)) {}

    ~scoped_file() {
        remove();
    }
};

scoped_file
create_temporary_file(std::size_t desired_size) {

    char name[] = "posix_file_tests_XXXXXX";
    posix_file::file_handle fh{mkstemp(name)};

    if(!fh) {
        fmt::print(stderr, "mkstemp() error: {}\n", strerror(errno));
        abort();
    }

    if(ftruncate(fh.native(), static_cast<off_t>(desired_size)) != 0) {
        fmt::print(stderr, "ftruncate() error: {}\n", strerror(errno));
        abort();
    }

    return scoped_file{std::filesystem::path{name}};
}

std::vector<posix_file::ranges::range>
generate_ranges(posix_file::offset start_offset, std::size_t length,
                std::size_t block_size, std::size_t file_size,
                unsigned int step = 1, unsigned int disp = 0) {

    assert(step != 0);

    std::size_t n =
            posix_file::math::block_count(start_offset, length, block_size);

    std::vector<posix_file::ranges::range> tmp;
    tmp.reserve(n);

    if(n == 0) {
        return tmp;
    }

    const auto block_index =
            posix_file::math::block_index(start_offset, block_size);

    for(auto i = block_index; i < block_index + n; ++i) {
        tmp.emplace_back(i * block_size, block_size);
    }

    // filter out any ranges beyond eof
    tmp.erase(std::remove_if(tmp.begin(), tmp.end(),
                             [eof = static_cast<posix_file::offset>(file_size)](
                                     const posix_file::ranges::range& r) {
                                 return r.offset() >= eof;
                             }),
              tmp.end());

    // adjust potential partial offset for first block
    if(start_offset != 0) {
        tmp.at(0) = posix_file::ranges::range{
                start_offset, block_size - posix_file::math::block_overrun(
                                                   start_offset, block_size)};
    }

    // adjust potential partial length for last block
    posix_file::offset end = std::min(
            static_cast<posix_file::offset>(file_size), start_offset + length);
    if(!posix_file::math::is_aligned(end, block_size)) {
        const auto r = tmp.back();
        tmp.back() = posix_file::ranges::range{r.offset(), end - r.offset()};
    }

    std::vector<posix_file::ranges::range> out;

    for(auto i = 0u; i < tmp.size(); ++i) {

        if(i < disp) {
            continue;
        }

        if((i - disp) % step == 0) {
            out.push_back(tmp[i]);
        }
    }

    return out;
}

SCENARIO("Generating ranges", "[posix_file][generate_ranges]") {

    GIVEN("An offset range") {
        WHEN("range length is 1") {
            WHEN("skip is 0") {
                WHEN("step is 1") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1},
                            {5, 1}, {6, 1}, {7, 1}, {8, 1}, {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 2") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 2);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 1},
                            {2, 1},
                            {4, 1},
                            {6, 1},
                            {8, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 3") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 3);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 1},
                            {3, 1},
                            {6, 1},
                            {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 4") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 4);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 1},
                            {4, 1},
                            {8, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 5") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 5);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 1},
                            {5, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 6") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 6);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 1},
                            {6, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 7") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 7);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 1},
                            {7, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 8") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 8);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 1},
                            {8, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 9") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 9);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 1},
                            {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is >9") {
                    const int step =
                            GENERATE(Catch::Generators::range(10, 100));
                    const auto ranges = generate_ranges(0, 10, 1, 100, step);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            WHEN("skip is 1") {
                WHEN("step is 1") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 1, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1},
                            {6, 1}, {7, 1}, {8, 1}, {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 2") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 2, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {1, 1},
                            {3, 1},
                            {5, 1},
                            {7, 1},
                            {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 3") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 3, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {1, 1},
                            {4, 1},
                            {7, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 4") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 4, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {1, 1},
                            {5, 1},
                            {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 5") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 5, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {1, 1},
                            {6, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 6") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 6, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {1, 1},
                            {7, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 7") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 7, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {1, 1},
                            {8, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 8") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 8, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {1, 1},
                            {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is >8") {
                    const int step = GENERATE(Catch::Generators::range(9, 100));
                    const auto ranges = generate_ranges(0, 10, 1, 100, step, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {1, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            WHEN("skip is 2") {
                WHEN("step is 1") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 1, 2);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 1}, {3, 1}, {4, 1}, {5, 1},
                            {6, 1}, {7, 1}, {8, 1}, {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 2") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 2, 2);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 1},
                            {4, 1},
                            {6, 1},
                            {8, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 3") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 3, 2);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 1},
                            {5, 1},
                            {8, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 4") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 4, 2);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 1},
                            {6, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 5") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 5, 2);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 1},
                            {7, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 6") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 6, 2);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 1},
                            {8, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 7") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 7, 2);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 1},
                            {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is >7") {
                    const int step = GENERATE(Catch::Generators::range(8, 100));
                    const auto ranges = generate_ranges(0, 10, 1, 100, step, 2);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            WHEN("skip is 3") {
                WHEN("step is 1") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 1, 3);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {3, 1}, {4, 1}, {5, 1}, {6, 1},
                            {7, 1}, {8, 1}, {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 2") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 2, 3);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {3, 1},
                            {5, 1},
                            {7, 1},
                            {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 3") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 3, 3);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {3, 1},
                            {6, 1},
                            {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 4") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 4, 3);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {3, 1},
                            {7, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 5") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 5, 3);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {3, 1},
                            {8, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 6") {
                    const auto ranges = generate_ranges(0, 10, 1, 100, 6, 3);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {3, 1},
                            {9, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is >6") {
                    const int step = GENERATE(Catch::Generators::range(7, 100));
                    const auto ranges = generate_ranges(0, 10, 1, 100, step, 3);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {3, 1}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            WHEN("skip is >9") {
                const int disp = GENERATE(Catch::Generators::range(10, 100));
                const int step = GENERATE(Catch::Generators::range(1, 100));

                CAPTURE(disp, step);

                const auto ranges = generate_ranges(0, 10, 1, 100, step, disp);
                std::vector<posix_file::ranges::range> expected_ranges{};

                THEN("Generated ranges are correct") {
                    REQUIRE(ranges == expected_ranges);
                }
            }
        }

        WHEN("range length is 2") {
            WHEN("skip is 0") {
                WHEN("step is 1") {
                    const auto ranges = generate_ranges(0, 10, 2, 100);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 2},
                            {2, 2},
                            {4, 2},
                            {6, 2},
                            {8, 2}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 2") {
                    const auto ranges = generate_ranges(0, 10, 2, 100, 2);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 2},
                            {4, 2},
                            {8, 2}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 3") {
                    const auto ranges = generate_ranges(0, 10, 2, 100, 3);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 2},
                            {6, 2},
                    };

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 4") {
                    const auto ranges = generate_ranges(0, 10, 2, 100, 4);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 2},
                            {8, 2},
                    };

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is >4") {
                    const int step = GENERATE(Catch::Generators::range(5, 100));
                    const auto ranges = generate_ranges(0, 10, 2, 100, step);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {0, 2}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            WHEN("skip is 1") {
                WHEN("step is 1") {
                    const auto ranges = generate_ranges(0, 10, 2, 100, 1, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 2},
                            {4, 2},
                            {6, 2},
                            {8, 2}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 2") {
                    const auto ranges = generate_ranges(0, 10, 2, 100, 2, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 2},
                            {6, 2}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is 3") {
                    const auto ranges = generate_ranges(0, 10, 2, 100, 3, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 2},
                            {8, 2}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                WHEN("step is >3") {
                    const int step = GENERATE(Catch::Generators::range(4, 100));
                    const auto ranges = generate_ranges(0, 10, 2, 100, step, 1);
                    std::vector<posix_file::ranges::range> expected_ranges{
                            {2, 2}};

                    THEN("Generated ranges are correct") {
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }
        }
    }
}

SCENARIO("Iterating over a file in byte units",
         "[posix_file][views][all_of][as_bytes]") {

    const std::size_t block_size = 1;
    CAPTURE(block_size);

    GIVEN("An empty file") {

        auto f = create_temporary_file(0);

        WHEN("Iterating over the file range") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r : all_of(f)) {
                ranges.emplace_back(r, 1);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }
    }

    GIVEN("A non-empty file larger than a byte") {

        const auto file_size = 2000u;
        auto f = create_temporary_file(file_size);

        CAPTURE(file_size);

        WHEN("Iterating over the file range") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r : all_of(f)) {
                ranges.emplace_back(r, 1);
            }

            THEN("Ranges for each byte are returned") {
                const auto expected_ranges =
                        generate_ranges(0, file_size, block_size, file_size);
                REQUIRE(ranges == expected_ranges);
            }
        }
    }
}

SCENARIO("Iterating over a file region in byte units",
         "[posix_file][views][some_of][as_bytes]") {

    const std::size_t block_size = 1;
    CAPTURE(block_size);

    GIVEN("An empty file") {

        auto f = create_temporary_file(0);

        WHEN("The region starts at the beginning of the file") {
            std::vector<posix_file::ranges::range> ranges;

            for(const auto& r : some_of(f, 0, 10)) {
                ranges.emplace_back(r, 1);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }

        WHEN("The region starts in the middle of the file") {
            std::vector<posix_file::ranges::range> ranges;

            for(const auto& r : some_of(f, 10, 10)) {
                ranges.emplace_back(r, 1);
            }

            CAPTURE(ranges);

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }
    }

    GIVEN("A non-empty file larger than a byte") {

        const auto file_size = 2000u;
        auto f = create_temporary_file(file_size);
        CAPTURE(file_size);

        WHEN("The region's length is 0") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r : some_of(f, 0, 0)) {
                ranges.emplace_back(r, 1);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }

        WHEN("The region's length is not 0") {
            WHEN("The region starts at the beginning of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = 0;
                std::size_t length = 10;
                CAPTURE(offset, length);

                for(const auto& r : some_of(f, offset, length)) {
                    ranges.emplace_back(r, 1);
                }

                THEN("Ranges for each byte are returned") {
                    const auto expected_ranges = generate_ranges(
                            offset, length, block_size, file_size);
                    REQUIRE(ranges == expected_ranges);
                }
            }

            WHEN("The region starts in the middle of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = 10;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r : some_of(f, offset, length)) {
                    ranges.emplace_back(r, 1);
                }

                THEN("Ranges for each byte are returned") {
                    const auto expected_ranges = generate_ranges(
                            offset, length, block_size, file_size);
                    REQUIRE(ranges == expected_ranges);
                }
            }

            WHEN("The region starts at the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r : some_of(f, offset, length)) {
                    ranges.emplace_back(r, 1);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }
        }
    }
}

SCENARIO("Iterating over a file in block units",
         "[posix_file][views][all_of][as_blocks]") {

    const std::size_t block_size = 512;
    CAPTURE(block_size);

    GIVEN("An empty file") {

        auto f = create_temporary_file(0);

        WHEN("Iterating over the file range") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r :
                all_of(f) | posix_file::views::as_blocks(block_size)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }
    }

    GIVEN("A non-empty file larger than a block") {

        const auto file_size = static_cast<std::size_t>(block_size * 1.5);
        auto f = create_temporary_file(file_size);
        CAPTURE(file_size);

        WHEN("Iterating over the file range") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r :
                all_of(f) | posix_file::views::as_blocks(block_size)) {
                ranges.push_back(r);
            }

            THEN("Ranges for each block are returned") {
                const auto expected_ranges =
                        generate_ranges(0, file_size, block_size, file_size);
                REQUIRE(ranges == expected_ranges);
            }
        }
    }

    GIVEN("A non-empty file smaller than a block") {

        const auto file_size = static_cast<std::size_t>(block_size / 1.5);
        CAPTURE(file_size);

        auto f = create_temporary_file(file_size);

        WHEN("Iterating over the file range") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r :
                all_of(f) | posix_file::views::as_blocks(block_size)) {
                ranges.push_back(r);
            }

            THEN("Ranges for each block are returned") {
                const auto expected_ranges =
                        generate_ranges(0, file_size, block_size, file_size);
                REQUIRE(ranges == expected_ranges);
            }
        }
    }

    GIVEN("A non-empty file of exactly one block") {

        const auto file_size = static_cast<std::size_t>(block_size);
        CAPTURE(file_size);

        auto f = create_temporary_file(file_size);

        WHEN("Iterating over the file range") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r :
                all_of(f) | posix_file::views::as_blocks(block_size)) {
                ranges.push_back(r);
            }

            THEN("Ranges for each block are returned") {
                const auto expected_ranges =
                        generate_ranges(0, file_size, block_size, file_size);
                REQUIRE(ranges == expected_ranges);
            }
        }
    }

    GIVEN("A non-empty file of exactly n blocks") {

        WHEN("n is a power of 2") {
            const auto n = 8;
            const auto file_size = static_cast<std::size_t>(n * block_size);
            CAPTURE(file_size);

            auto f = create_temporary_file(file_size);

            WHEN("Iterating over the file range") {
                std::vector<posix_file::ranges::range> ranges;
                for(const auto& r :
                    all_of(f) | posix_file::views::as_blocks(block_size)) {
                    ranges.push_back(r);
                }

                THEN("Ranges for each block are returned") {
                    const auto expected_ranges = generate_ranges(
                            0, file_size, block_size, file_size);
                    REQUIRE(ranges == expected_ranges);
                }
            }
        }

        WHEN("n is not a power of 2") {
            const auto n = 11;
            const auto file_size = static_cast<std::size_t>(n * block_size);
            CAPTURE(file_size);

            auto f = create_temporary_file(file_size);

            WHEN("Iterating over the file range") {
                std::vector<posix_file::ranges::range> ranges;
                for(const auto& r :
                    all_of(f) | posix_file::views::as_blocks(block_size)) {
                    ranges.push_back(r);
                }

                THEN("Ranges for each block are returned") {
                    const auto expected_ranges = generate_ranges(
                            0, file_size, block_size, file_size);
                    REQUIRE(ranges == expected_ranges);
                }
            }
        }
    }
}

SCENARIO("Iterating over a file region in block units",
         "[posix_file][views][some_of][as_blocks]") {

    const std::size_t block_size = 512;
    CAPTURE(block_size);

    GIVEN("An empty file") {

        auto f = create_temporary_file(0);

        WHEN("The region starts at the beginning of the file") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r :
                some_of(f, 0, 10) | posix_file::views::as_blocks(block_size)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }

        WHEN("The region starts in the middle of the file") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r :
                some_of(f, 0, 10) | posix_file::views::as_blocks(block_size)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }
    }

    GIVEN("A non-empty file larger than a block") {

        const auto file_size = static_cast<std::size_t>(block_size * 1.5);
        auto f = create_temporary_file(file_size);
        CAPTURE(file_size);

        WHEN("The region's length is 0") {
            std::vector<posix_file::ranges::range> ranges;

            posix_file::offset offset = 0;
            std::size_t length = 0;
            CAPTURE(offset, length);

            for(const auto& r :
                some_of(f, offset, length) |
                        posix_file::views::as_blocks(block_size)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }

        WHEN("The region's length is not 0") {
            AND_WHEN("The region starts at the beginning of the file") {
                AND_WHEN("The region does not end at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 10;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 3.5 * block_size;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts in the middle of the file") {
                AND_WHEN("The region does not end at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            static_cast<std::size_t>(3.5 * block_size) -
                            static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            file_size - static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends beyond EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = file_size * 4;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts at the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }

            AND_WHEN("The region starts beyond the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size + 1;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }
        }
    }

    GIVEN("A non-empty file smaller than a block") {

        const auto file_size = static_cast<std::size_t>(block_size / 1.5);
        CAPTURE(file_size);

        auto f = create_temporary_file(file_size);

        WHEN("The region's length is 0") {
            std::vector<posix_file::ranges::range> ranges;

            posix_file::offset offset = 0;
            std::size_t length = 0;
            CAPTURE(offset, length);

            for(const auto& r :
                some_of(f, offset, length) |
                        posix_file::views::as_blocks(block_size)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }

        WHEN("The region's length is not 0") {
            AND_WHEN("The region starts at the beginning of the file") {
                AND_WHEN("The region does not end at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 10;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 3.5 * block_size;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts in the middle of the file") {
                AND_WHEN("The region does not end at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            static_cast<std::size_t>(3.5 * block_size) -
                            static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            file_size - static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends beyond EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = file_size * 4;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts at the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }

            AND_WHEN("The region starts beyond the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size + 1;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }
        }
    }

    GIVEN("A non-empty file of exactly one block") {

        const auto file_size = static_cast<std::size_t>(block_size);
        CAPTURE(file_size);

        auto f = create_temporary_file(file_size);

        WHEN("The region's length is 0") {
            std::vector<posix_file::ranges::range> ranges;

            posix_file::offset offset = 0;
            std::size_t length = 0;
            CAPTURE(offset, length);

            for(const auto& r :
                some_of(f, offset, length) |
                        posix_file::views::as_blocks(block_size)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }

        WHEN("The region's length is not 0") {
            AND_WHEN("The region starts at the beginning of the file") {
                AND_WHEN("The region does not end at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 10;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 3.5 * block_size;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts in the middle of the file") {
                AND_WHEN("The region does not end at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            static_cast<std::size_t>(3.5 * block_size) -
                            static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            file_size - static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends beyond EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = file_size * 4;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges = generate_ranges(
                                offset, length, block_size, file_size);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts at the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }

            AND_WHEN("The region starts beyond the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size + 1;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }
        }
    }

    GIVEN("A non-empty file of exactly n blocks") {

        WHEN("n is a power of 2") {
            const auto n = 8;
            const auto file_size = static_cast<std::size_t>(n * block_size);
            CAPTURE(file_size);

            auto f = create_temporary_file(file_size);

            AND_WHEN("The region's length is 0") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = 0;
                std::size_t length = 0;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }

            AND_WHEN("The region's length is not 0") {
                AND_WHEN("The region starts at the beginning of the file") {
                    AND_WHEN("The region does not end at a block boundary") {
                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 0;
                        std::size_t length = 10;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at a block boundary") {
                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 0;
                        std::size_t length = 3.5 * block_size;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }
                }

                AND_WHEN("The region starts in the middle of the file") {
                    AND_WHEN("The region does not end at a block boundary") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length = 20;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at a block boundary") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length =
                                static_cast<std::size_t>(3.5 * block_size) -
                                static_cast<std::size_t>(offset);
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at EOF") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length =
                                file_size - static_cast<std::size_t>(offset);
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends beyond EOF") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length = file_size * 4;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }
                }

                AND_WHEN("The region starts at the end of the file") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = file_size;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("No ranges are returned") {
                        REQUIRE(ranges.empty());
                    }
                }

                AND_WHEN("The region starts beyond the end of the file") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = file_size + 1;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("No ranges are returned") {
                        REQUIRE(ranges.empty());
                    }
                }
            }
        }

        WHEN("n is not a power of 2") {
            const auto n = 11;
            const auto file_size = static_cast<std::size_t>(n * block_size);
            CAPTURE(file_size);

            auto f = create_temporary_file(file_size);

            AND_WHEN("The region's length is 0") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = 0;
                std::size_t length = 0;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }

            AND_WHEN("The region's length is not 0") {
                AND_WHEN("The region starts at the beginning of the file") {
                    AND_WHEN("The region does not end at a block boundary") {
                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 0;
                        std::size_t length = 10;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at a block boundary") {
                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 0;
                        std::size_t length = 3.5 * block_size;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }
                }

                AND_WHEN("The region starts in the middle of the file") {
                    AND_WHEN("The region does not end at a block boundary") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length = 20;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at a block boundary") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length =
                                static_cast<std::size_t>(3.5 * block_size) -
                                static_cast<std::size_t>(offset);
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at EOF") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length =
                                file_size - static_cast<std::size_t>(offset);
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends beyond EOF") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length = file_size * 4;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges = generate_ranges(
                                    offset, length, block_size, file_size);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }
                }

                AND_WHEN("The region starts at the end of the file") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = file_size;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("No ranges are returned") {
                        REQUIRE(ranges.empty());
                    }
                }

                AND_WHEN("The region starts beyond the end of the file") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = file_size + 1;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size)) {
                        ranges.push_back(r);
                    }

                    THEN("No ranges are returned") {
                        REQUIRE(ranges.empty());
                    }
                }
            }
        }
    }
}

SCENARIO("Iterating over a file in block units with stride",
         "[posix_file][views][all_of][strided]") {

    const std::size_t block_size = 512;
    const std::size_t step = GENERATE(Catch::Generators::range(1, 50));
    const std::size_t disp = GENERATE(Catch::Generators::range(0, 10));
    CAPTURE(block_size, step, disp);

    GIVEN("An empty file") {

        auto f = create_temporary_file(0);

        WHEN("Iterating over the file range") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r :
                all_of(f) | as_blocks(block_size) | strided(step, disp)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }
    }

    GIVEN("A non-empty file larger than a block") {

        const auto file_size = static_cast<std::size_t>(block_size * 1.5);
        auto f = create_temporary_file(file_size);
        CAPTURE(file_size);

        WHEN("Iterating over the file range") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r :
                all_of(f) | as_blocks(block_size) | strided(step, disp)) {
                ranges.push_back(r);
            }

            THEN("Ranges for each block are returned") {
                const auto expected_ranges = generate_ranges(
                        0, file_size, block_size, file_size, step, disp);
                REQUIRE(ranges == expected_ranges);
            }
        }
    }

    GIVEN("A non-empty file smaller than a block") {

        const auto file_size = static_cast<std::size_t>(block_size / 1.5);
        CAPTURE(file_size);

        auto f = create_temporary_file(file_size);

        WHEN("Iterating over the file range") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r :
                all_of(f) | as_blocks(block_size) | strided(step, disp)) {
                ranges.push_back(r);
            }

            THEN("Ranges for each block are returned") {
                const auto expected_ranges = generate_ranges(
                        0, file_size, block_size, file_size, step, disp);
                REQUIRE(ranges == expected_ranges);
            }
        }
    }

    GIVEN("A non-empty file of exactly one block") {

        const auto file_size = static_cast<std::size_t>(block_size);
        CAPTURE(file_size);

        auto f = create_temporary_file(file_size);

        WHEN("Iterating over the file range") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r :
                all_of(f) | as_blocks(block_size) | strided(step, disp)) {
                ranges.push_back(r);
            }

            THEN("Ranges for each block are returned") {
                const auto expected_ranges = generate_ranges(
                        0, file_size, block_size, file_size, step, disp);
                REQUIRE(ranges == expected_ranges);
            }
        }
    }

    GIVEN("A non-empty file of exactly n blocks") {

        WHEN("n is a power of 2") {
            const auto n = 8;
            const auto file_size = static_cast<std::size_t>(n * block_size);
            CAPTURE(file_size);

            auto f = create_temporary_file(file_size);

            WHEN("Iterating over the file range") {
                std::vector<posix_file::ranges::range> ranges;
                for(const auto& r :
                    all_of(f) | as_blocks(block_size) | strided(step, disp)) {
                    ranges.push_back(r);
                }

                THEN("Ranges for each block are returned") {
                    const auto expected_ranges = generate_ranges(
                            0, file_size, block_size, file_size, step, disp);
                    REQUIRE(ranges == expected_ranges);
                }
            }
        }

        WHEN("n is not a power of 2") {
            const auto n = 11;
            const auto file_size = static_cast<std::size_t>(n * block_size);
            CAPTURE(file_size);

            auto f = create_temporary_file(file_size);

            WHEN("Iterating over the file range") {
                std::vector<posix_file::ranges::range> ranges;
                for(const auto& r :
                    all_of(f) | as_blocks(block_size) | strided(step, disp)) {
                    ranges.push_back(r);
                }

                THEN("Ranges for each block are returned") {
                    const auto expected_ranges = generate_ranges(
                            0, file_size, block_size, file_size, step, disp);
                    REQUIRE(ranges == expected_ranges);
                }
            }
        }
    }
}

SCENARIO("Iterating over a file region in block units with stride",
         "[posix_file][views][some_of][strided]") {

    const std::size_t block_size = 512;
    const std::size_t step = GENERATE(Catch::Generators::range(1, 50));
    const std::size_t disp = GENERATE(Catch::Generators::range(0, 10));
    CAPTURE(block_size, step, disp);

    GIVEN("An empty file") {

        auto f = create_temporary_file(0);

        WHEN("The region starts at the beginning of the file") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r : some_of(f, 0, 10) | as_blocks(block_size) |
                                        strided(step, disp)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }

        WHEN("The region starts in the middle of the file") {
            std::vector<posix_file::ranges::range> ranges;
            for(const auto& r : some_of(f, 0, 10) | as_blocks(block_size) |
                                        strided(step, disp)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }
    }

    GIVEN("A non-empty file larger than a block") {

        const auto file_size = static_cast<std::size_t>(block_size * 1.5);
        auto f = create_temporary_file(file_size);
        CAPTURE(file_size);

        WHEN("The region's length is 0") {
            std::vector<posix_file::ranges::range> ranges;

            posix_file::offset offset = 0;
            std::size_t length = 0;
            CAPTURE(offset, length);

            for(const auto& r : some_of(f, offset, length) |
                                        as_blocks(block_size) |
                                        strided(step, disp)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }

        WHEN("The region's length is not 0") {
            AND_WHEN("The region starts at the beginning of the file") {
                AND_WHEN("The region does not end at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 10;
                    CAPTURE(offset, length);

                    for(const auto& r : some_of(f, offset, length) |
                                                as_blocks(block_size) |
                                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 3.5 * block_size;
                    CAPTURE(offset, length);

                    for(const auto& r : some_of(f, offset, length) |
                                                as_blocks(block_size) |
                                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts in the middle of the file") {
                AND_WHEN("The region does not end at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r : some_of(f, offset, length) |
                                                as_blocks(block_size) |
                                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            static_cast<std::size_t>(3.5 * block_size) -
                            static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r : some_of(f, offset, length) |
                                                as_blocks(block_size) |
                                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            file_size - static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r : some_of(f, offset, length) |
                                                as_blocks(block_size) |
                                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends beyond EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = file_size * 4;
                    CAPTURE(offset, length);

                    for(const auto& r : some_of(f, offset, length) |
                                                as_blocks(block_size) |
                                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts at the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r : some_of(f, offset, length) |
                                            as_blocks(block_size) |
                                            strided(step, disp)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }

            AND_WHEN("The region starts beyond the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size + 1;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r : some_of(f, offset, length) |
                                            as_blocks(block_size) |
                                            strided(step, disp)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }
        }
    }

    GIVEN("A non-empty file smaller than a block") {

        const auto file_size = static_cast<std::size_t>(block_size / 1.5);
        CAPTURE(file_size);

        auto f = create_temporary_file(file_size);

        WHEN("The region's length is 0") {
            std::vector<posix_file::ranges::range> ranges;

            posix_file::offset offset = 0;
            std::size_t length = 0;
            CAPTURE(offset, length);

            for(const auto& r : some_of(f, offset, length) |
                                        as_blocks(block_size) |
                                        strided(step, disp)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }

        WHEN("The region's length is not 0") {
            AND_WHEN("The region starts at the beginning of the file") {
                AND_WHEN("The region does not end at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 10;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 3.5 * block_size;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts in the middle of the file") {
                AND_WHEN("The region does not end at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            static_cast<std::size_t>(3.5 * block_size) -
                            static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            file_size - static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends beyond EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = file_size * 4;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts at the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size) |
                            strided(step, disp)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }

            AND_WHEN("The region starts beyond the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size + 1;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size) |
                            strided(step, disp)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }
        }
    }

    GIVEN("A non-empty file of exactly one block") {

        const auto file_size = static_cast<std::size_t>(block_size);
        CAPTURE(file_size);

        auto f = create_temporary_file(file_size);

        WHEN("The region's length is 0") {
            std::vector<posix_file::ranges::range> ranges;

            posix_file::offset offset = 0;
            std::size_t length = 0;
            CAPTURE(offset, length);

            for(const auto& r :
                some_of(f, offset, length) |
                        posix_file::views::as_blocks(block_size) |
                        strided(step, disp)) {
                ranges.push_back(r);
            }

            THEN("No ranges are returned") {
                REQUIRE(ranges.empty());
            }
        }

        WHEN("The region's length is not 0") {
            AND_WHEN("The region starts at the beginning of the file") {
                AND_WHEN("The region does not end at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 10;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 0;
                    std::size_t length = 3.5 * block_size;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts in the middle of the file") {
                AND_WHEN("The region does not end at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at a block boundary") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            static_cast<std::size_t>(3.5 * block_size) -
                            static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends at EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length =
                            file_size - static_cast<std::size_t>(offset);
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }

                AND_WHEN("The region ends beyond EOF") {

                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = 10;
                    std::size_t length = file_size * 4;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("Ranges for each block are returned") {
                        const auto expected_ranges =
                                generate_ranges(offset, length, block_size,
                                                file_size, step, disp);
                        REQUIRE(ranges == expected_ranges);
                    }
                }
            }

            AND_WHEN("The region starts at the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size) |
                            strided(step, disp)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }

            AND_WHEN("The region starts beyond the end of the file") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = file_size + 1;
                std::size_t length = 20;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size) |
                            strided(step, disp)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }
        }
    }

    GIVEN("A non-empty file of exactly n blocks") {

        WHEN("n is a power of 2") {
            const auto n = 8;
            const auto file_size = static_cast<std::size_t>(n * block_size);
            CAPTURE(file_size);

            auto f = create_temporary_file(file_size);

            AND_WHEN("The region's length is 0") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = 0;
                std::size_t length = 0;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size) |
                            strided(step, disp)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }

            AND_WHEN("The region's length is not 0") {
                AND_WHEN("The region starts at the beginning of the file") {
                    AND_WHEN("The region does not end at a block boundary") {
                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 0;
                        std::size_t length = 10;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at a block boundary") {
                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 0;
                        std::size_t length = 3.5 * block_size;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }
                }

                AND_WHEN("The region starts in the middle of the file") {
                    AND_WHEN("The region does not end at a block boundary") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length = 20;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at a block boundary") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length =
                                static_cast<std::size_t>(3.5 * block_size) -
                                static_cast<std::size_t>(offset);
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at EOF") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length =
                                file_size - static_cast<std::size_t>(offset);
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends beyond EOF") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length = file_size * 4;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }
                }

                AND_WHEN("The region starts at the end of the file") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = file_size;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("No ranges are returned") {
                        REQUIRE(ranges.empty());
                    }
                }

                AND_WHEN("The region starts beyond the end of the file") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = file_size + 1;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("No ranges are returned") {
                        REQUIRE(ranges.empty());
                    }
                }
            }
        }

        WHEN("n is not a power of 2") {
            const auto n = 11;
            const auto file_size = static_cast<std::size_t>(n * block_size);
            CAPTURE(file_size);

            auto f = create_temporary_file(file_size);

            AND_WHEN("The region's length is 0") {
                std::vector<posix_file::ranges::range> ranges;

                posix_file::offset offset = 0;
                std::size_t length = 0;
                CAPTURE(offset, length);

                for(const auto& r :
                    some_of(f, offset, length) |
                            posix_file::views::as_blocks(block_size) |
                            strided(step, disp)) {
                    ranges.push_back(r);
                }

                THEN("No ranges are returned") {
                    REQUIRE(ranges.empty());
                }
            }

            AND_WHEN("The region's length is not 0") {
                AND_WHEN("The region starts at the beginning of the file") {
                    AND_WHEN("The region does not end at a block boundary") {
                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 0;
                        std::size_t length = 10;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at a block boundary") {
                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 0;
                        std::size_t length = 3.5 * block_size;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }
                }

                AND_WHEN("The region starts in the middle of the file") {
                    AND_WHEN("The region does not end at a block boundary") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length = 20;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at a block boundary") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length =
                                static_cast<std::size_t>(3.5 * block_size) -
                                static_cast<std::size_t>(offset);
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends at EOF") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length =
                                file_size - static_cast<std::size_t>(offset);
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }

                    AND_WHEN("The region ends beyond EOF") {

                        std::vector<posix_file::ranges::range> ranges;

                        posix_file::offset offset = 10;
                        std::size_t length = file_size * 4;
                        CAPTURE(offset, length);

                        for(const auto& r :
                            some_of(f, offset, length) |
                                    posix_file::views::as_blocks(block_size) |
                                    strided(step, disp)) {
                            ranges.push_back(r);
                        }

                        THEN("Ranges for each block are returned") {
                            const auto expected_ranges =
                                    generate_ranges(offset, length, block_size,
                                                    file_size, step, disp);
                            REQUIRE(ranges == expected_ranges);
                        }
                    }
                }

                AND_WHEN("The region starts at the end of the file") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = file_size;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("No ranges are returned") {
                        REQUIRE(ranges.empty());
                    }
                }

                AND_WHEN("The region starts beyond the end of the file") {
                    std::vector<posix_file::ranges::range> ranges;

                    posix_file::offset offset = file_size + 1;
                    std::size_t length = 20;
                    CAPTURE(offset, length);

                    for(const auto& r :
                        some_of(f, offset, length) |
                                posix_file::views::as_blocks(block_size) |
                                strided(step, disp)) {
                        ranges.push_back(r);
                    }

                    THEN("No ranges are returned") {
                        REQUIRE(ranges.empty());
                    }
                }
            }
        }
    }
}
