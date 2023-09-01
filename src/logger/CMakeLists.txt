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

# get the parent directory of the current directory so we can include
# headers from these libraries as `<logger/*.hpp>`
get_filename_component(PARENT_DIRECTORY "../" ABSOLUTE)

add_library(logger STATIC)
target_sources(
  logger
  INTERFACE logger.h logger.hpp macros.h
  PRIVATE logger.cpp
)

target_include_directories(logger PUBLIC ${PARENT_DIRECTORY})
target_link_libraries(logger PUBLIC spdlog::spdlog fmt::fmt)
set_property(TARGET logger PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(logger::logger ALIAS logger)
