################################################################################
# Copyright 2022-2023, Barcelona Supercomputing Center (BSC), Spain            #
#                                                                              #
# This software was partially supported by the EuroHPC-funded project ADMIRE   #
#   (Project ID: 956748, https://www.admire-eurohpc.eu).                       #
#                                                                              #
# This file is part of Cargo.                                                  #
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


include(CMakeParseArguments)

#[=======================================================================[.rst:

  get_systemd_unit_directory(OUTPUT_VARIABLE [USER])

Initialize ``OUTPUT_VARIABLE`` to the directory where systemd unit files
are installed. This function will use ``pkg-config`` to find the information.
If ``USER`` is specified, the user unit directory will be returned instead.
#]=======================================================================]
function(get_systemd_unit_directory OUTPUT_VARIABLE)

  set(OPTIONS USER)
  set(SINGLE_VALUE)
  set(MULTI_VALUE)

  cmake_parse_arguments(
    ARGS "${OPTIONS}" "${SINGLE_VALUE}" "${MULTI_VALUE}" ${ARGN}
  )

  if(ARGS_UNPARSED_ARGUMENTS)
    message(WARNING "Unparsed arguments in get_systemd_unit_directory(): "
      "this often indicates typos!\n"
      "Unparsed arguments: ${ARGS_UNPARSED_ARGUMENTS}"
    )
  endif()

  find_package(PkgConfig REQUIRED)

  # Check for the systemd application, so that we can find out where to
  # install the service unit files
  pkg_check_modules(SYSTEMD_PROGRAM QUIET systemd)

  if (ARGS_USER)
    set(_pc_var systemduserunitdir)
  else ()
    set(_pc_var systemdsystemunitdir)
  endif ()

  if (SYSTEMD_PROGRAM_FOUND)
    # Use pkg-config to look up the systemd unit install directory
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE}
      --variable=${_pc_var} systemd
      OUTPUT_VARIABLE _systemd_unit_dir)
    string(REGEX REPLACE "[ \t\n]+" "" _systemd_unit_dir "${_systemd_unit_dir}")

    message(STATUS "systemd services install dir: ${_systemd_unit_dir}")

    set(${OUTPUT_VARIABLE}
      ${_systemd_unit_dir}
      PARENT_SCOPE
    )
  endif ()
endfunction()
