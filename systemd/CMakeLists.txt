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

configure_file(cargo@.service.in cargo@.service @ONLY)

# If requested, install the systemd unit files to the system directory
# for user-defined services. Otherwise, install them to the configured
# `datadir` directory (usually `/usr/share`).
if(CARGO_SYSTEMD_INSTALL_UNIT_FILES)
  include(systemd)
  get_systemd_unit_directory(SYSTEMD_UNIT_DIRECTORY USER)

  if(NOT SYSTEMD_UNIT_DIRECTORY)
    message(FATAL_ERROR "Could not find systemd unit directory")
  endif()

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cargo@.service
          DESTINATION ${SYSTEMD_UNIT_DIRECTORY}
  )
else()
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cargo@.service
          DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}
  )
endif()
