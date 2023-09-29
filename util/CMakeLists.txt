################################################################################
# Copyright 2022-2023, Barcelona Supercomputing Center (BSC), Spain            #
#                                                                              #
# This software was partially supported by the EuroHPC-funded project ADMIRE   #
#   (Project ID: 956748, https://www.admire-eurohpc.eu).                       #
#                                                                              #
# This file is part of Cargo.                                                  #
#                                                                              #
# Cargo is free software: you can redistribute it and/or modify                #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation, either version 3 of the License, or            #
# (at your option) any later version.                                          #
#                                                                              #
# Cargo is distributed in the hope that it will be useful,                     #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with Cargo.  If not, see <https://www.gnu.org/licenses/>.              #
#                                                                              #
# SPDX-License-Identifier: GPL-3.0-or-later                                    #
################################################################################

add_executable(cargo_ping)

target_sources(cargo_ping
  PRIVATE
    ping.cpp
)

target_link_libraries(cargo_ping
  PUBLIC
    fmt::fmt
    CLI11::CLI11
    net::rpc_client
    cargo
)

add_executable(cargo_shutdown)

target_sources(cargo_shutdown
  PRIVATE
    shutdown.cpp
)

target_link_libraries(cargo_shutdown
        PUBLIC
        fmt::fmt
        CLI11::CLI11
        net::rpc_client
        cargo
)

install(TARGETS cargo_ping cargo_shutdown
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
