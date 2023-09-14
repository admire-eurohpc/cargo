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

add_library(cargo SHARED)

target_sources(cargo PRIVATE cargo.hpp fmt_formatters.hpp cargo/error.hpp
        libcargo.cpp error.cpp)

list(APPEND public_headers "cargo.hpp;cargo/error.hpp")
list(APPEND public_headers "fmt_formatters.hpp")

target_include_directories(
  cargo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
               $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

set_target_properties(cargo PROPERTIES PUBLIC_HEADER "${public_headers}")

target_link_libraries(cargo PRIVATE
        logger::logger
        fmt::fmt
        thallium
        MPI::MPI_CXX
        Boost::serialization
        Boost::mpi
)

## Install library + targets ###################################################

set_target_properties(
  cargo PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION
                                              ${PROJECT_VERSION_MAJOR}
)

install(
  TARGETS cargo
  EXPORT cargoExports
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

install(
  EXPORT cargoExports
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}
  NAMESPACE cargo::
)
