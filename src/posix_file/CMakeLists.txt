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

add_library(posix_file STATIC)
set(GEKKO_INCLUDES "")

if (GEKKOFS_PLUGIN)
   set(GEKKO_INCLUDES posix_file/fs_plugin/gekko_plugin.hpp posix_file/fs_plugin/gekko_plugin.cpp)           
endif()
if (EXPAND_plugin)
   set(EXPAND_INCLUDES posix_file/fs_plugin/expand_plugin.hpp posix_file/fs_plugin/expand_plugin.cpp)
endif()
if (HERCULES_plugin)
   set(HERCULES_INCLUDES posix_file/fs_plugin/hercules_plugin.chpp posix_file/fs_plugin/hercules_plugin.cpp)
endif()


target_sources(
  posix_file
  PRIVATE posix_file/types.hpp
            posix_file/file.hpp
            posix_file/ranges.hpp
            posix_file/views.hpp
            posix_file/math.hpp
            posix_file/views/block_iterator.hpp
            posix_file/views/strided_iterator.hpp
            posix_file/fs_plugin/fs_plugin.hpp
            posix_file/fs_plugin/posix_plugin.hpp
            posix_file/fs_plugin/fs_plugin.cpp
            posix_file/fs_plugin/posix_plugin.cpp
            ${GEKKO_INCLUDES}
            ${HERCULES_INCLUDES}
            ${EXPAND_INCLUDES}
            
)


target_include_directories(posix_file INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET posix_file PROPERTY POSITION_INDEPENDENT_CODE ON)
set(ADHOC "")

if (GEKKOFS_PLUGIN)
   set(ADHOC ${ADHOC} GekkoFS::GekkoFS)
endif()

if (EXPAND_PLUGIN)
   set(ADHOC ${ADHOC} Expand::Expand)
endif()

if (HERCULES_PLUGIN)
   set(ADHOC ${ADHOC} Hercules::Hercules)
endif()


target_link_libraries(posix_file INTERFACE fmt::fmt tl::expected PRIVATE ${ADHOC})

