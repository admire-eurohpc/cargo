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


find_path(GekkoFS_INCLUDE_DIR
  NAMES user_functions.hpp
  PREFIX gkfs
)

find_library(GekkoFS_LIBRARY
  NAMES libgkfs_user_lib.so
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	GekkoFS
	DEFAULT_MSG
	GekkoFS_INCLUDE_DIR
	GekkoFS_LIBRARY
)

if(GekkoFS_FOUND)
  set(GekkoFS_LIBRARIES ${GekkoFS_LIBRARY})
  set(GekkoFS_INCLUDE_DIRS ${GekkoFS_INCLUDE_DIR})


  if(NOT TARGET GekkoFS::GekkoFS)
	  add_library(GekkoFS::GekkoFS UNKNOWN IMPORTED)
	  set_target_properties(GekkoFS::GekkoFS PROPERTIES
		IMPORTED_LOCATION "${GekkoFS_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${GekkoFS_INCLUDE_DIR}"
	  )
	endif()
endif()


mark_as_advanced(
	GekkoFS_INCLUDE_DIR
	GekkoFS_LIBRARY
)
