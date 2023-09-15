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

add_subdirectory(logger)
add_subdirectory(net)
add_subdirectory(posix_file)

## The main executable for the Cargo data stager
add_executable(cargo_server)

target_sources(
  cargo_server
  PRIVATE cargo.cpp
          master.cpp
          master.hpp
          worker.cpp
          worker.hpp
          env.hpp
          mpioxx.hpp
          request.cpp
          request.hpp
          request_manager.cpp
          request_manager.hpp
          shared_mutex.hpp
          proto/rpc/response.hpp
          proto/mpi/message.hpp
)

target_include_directories(
  cargo_server
  PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${CMAKE_BINARY_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(
  cargo_server
  PRIVATE config
          rpc_server
          cargo
          fmt::fmt
          MPI::MPI_CXX
          CLI11::CLI11
          Boost::serialization
          Boost::mpi
          posix_file
)

set_target_properties(cargo_server PROPERTIES OUTPUT_NAME "cargo")

install(TARGETS cargo_server DESTINATION ${CMAKE_INSTALL_BINDIR})
