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


find_path(DataClay_INCLUDE_DIR
  NAMES dataclayplugin.h
  PREFIX dataclay-plugin
)

find_path(DataClay_MODEL_DIR
  NAMES client.py
  PREFIX dataclay-plugin
)
message(STATUS "[${PROJECT_NAME}] DataClay library MODEL DIR ${DataClay_MODEL_DIR}")




find_library(DataClay_LIBRARY
  NAMES dataclay-plugin/libdataclayplugin.so
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	DataClay
	DEFAULT_MSG
	DataClay_INCLUDE_DIR
	DataClay_LIBRARY
	DataClay_MODEL_DIR
)

if(DataClay_FOUND)
  find_package(Python3 REQUIRED Development)
  message(STATUS "[${PROJECT_NAME}] DataClay library needs Python include ${Python3_INCLUDE_DIRS}")

  set(DataClay_LIBRARIES ${DataClay_LIBRARY})
  set(DataClay_INCLUDE_DIRS ${DataClay_INCLUDE_DIR} )


  if(NOT TARGET DataClay::DataClay)
	  add_library(DataClay::DataClay UNKNOWN IMPORTED)
	  set_target_properties(DataClay::DataClay PROPERTIES
		IMPORTED_LOCATION "${DataClay_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${DataClay_INCLUDE_DIR};${Python3_INCLUDE_DIRS}"
	  )
	endif()
endif()


mark_as_advanced(
	DataClay_INCLUDE_DIR
	DataClay_LIBRARY
)
