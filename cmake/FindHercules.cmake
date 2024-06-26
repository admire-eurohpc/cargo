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


find_path(Hercules_INCLUDE_DIR
  NAMES user_functions.hpp
  PREFIX hercules
)

find_library(Hercules_LIBRARY
  NAMES libhercules_user_lib.so
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	Hercules
	DEFAULT_MSG
	Hercules_INCLUDE_DIR
	Hercules_LIBRARY
)

if(Hercules_FOUND)
  set(Hercules_LIBRARIES ${Hercules_LIBRARY})
  set(Hercules_INCLUDE_DIRS ${Hercules_INCLUDE_DIR})


  if(NOT TARGET Hercules::Hercules)
	  add_library(Hercules::Hercules UNKNOWN IMPORTED)
	  set_target_properties(Hercules::Hercules PROPERTIES
		IMPORTED_LOCATION "${Hercules_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${Hercules_INCLUDE_DIR}"
	  )
	endif()
endif()


mark_as_advanced(
	Hercules_INCLUDE_DIR
	Hercules_LIBRARY
)
