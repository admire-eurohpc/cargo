################################################################################
# Copyright 2022-2023, Barcelona Supercomputing Center (BSC), Spain            #
#                                                                              #
# This software was partially supported by the EuroHPC-funded project ADMIRE   #
#   (Project ID: 956748, https://www.admire-eurohpc.eu).                       #
#                                                                              #
# This file is part of cargo.                                                  #
#                                                                              #
# cargo is free software: you can redistribute it and/or modify                #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation, either version 3 of the License, or            #
# (at your option) any later version.                                          #
#                                                                              #
# cargo is distributed in the hope that it will be useful,                     #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with cargo.  If not, see <https://www.gnu.org/licenses/>.              #
#                                                                              #
# SPDX-License-Identifier: GPL-3.0-or-later                                    #
################################################################################

include(Catch)

add_executable(tests)

target_sources(tests PRIVATE tests.cpp common.hpp common.cpp)

target_link_libraries(
  tests PUBLIC Catch2::Catch2 Boost::iostreams fmt::fmt cargo
)

catch_discover_tests(
  tests EXTRA_ARGS "-S ${CARGO_TRANSPORT_PROTOCOL}://${CARGO_BIND_ADDRESS}:${CARGO_BIND_PORT}"
)
