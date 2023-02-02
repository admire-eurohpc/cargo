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

add_library(posix_file INTERFACE)

target_sources(
  posix_file
  INTERFACE posix_file/types.hpp
            posix_file/file.hpp
            posix_file/ranges.hpp
            posix_file/views.hpp
            posix_file/math.hpp
            posix_file/views/block_iterator.hpp
            posix_file/views/strided_iterator.hpp
)

target_include_directories(posix_file INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET posix_file PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(posix_file INTERFACE fmt::fmt tl::expected)
