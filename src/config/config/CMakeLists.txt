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

# Create a config target for all configuration code
add_library(config STATIC)

target_include_directories(
  config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

target_sources(
  config
  PRIVATE config.hpp
          defaults.hpp
          ${CMAKE_CURRENT_BINARY_DIR}/defaults.cpp
          ${CMAKE_CURRENT_BINARY_DIR}/config_options.hpp
          ${CMAKE_CURRENT_BINARY_DIR}/keywords.hpp
          parsers.cpp
          parsers.hpp
          settings.cpp
          settings.hpp
)

target_link_libraries(config PRIVATE utils file_options::file_options)

# ##############################################################################
# Produce several auto-generated files for 'config'
# ##############################################################################

# Default values for options
configure_file(defaults.cpp.in defaults.cpp)

# Automatic generation of file_options schemas. To facilitate the management of
# any file-based configuration options, we generate a C++ schema from a YAML
# file that allows the 'file_options' library to parse them fromm a
# configuration file. The process works as follows:
#  1. We define the desired options in 'file_options.yml'
#  2. We rely on the 'genopts' tool to generate valid C++ schemas for the
#     'file_options' library. This tool can be configured using a 'genopts.yml'
#     configuration file.
#  3. In the daemon code, we use the facilities provided by the 'file_options'
#     library to access the options values.

# Since the configuration in genopts.yml can be path-dependant, we rely on
# CMake to substitute any special @variables@ for their actual values
configure_file(genopts.yml.in genopts.yml @ONLY)

# Define the command that will generate config_options.hpp and keywords.hpp.
# It will be executed since there is a direct dependency between 'config'
# (defined below) and these output files.
# We also make the command depend on file_options.yml and genopts.yml so that
# it gets re-executed if they change.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config_options.hpp
         ${CMAKE_CURRENT_BINARY_DIR}/keywords.hpp
  COMMENT "Generating config_options.hpp, keywords.hpp"
  DEPENDS genopts_virtualenv file_options.yml
          ${CMAKE_CURRENT_BINARY_DIR}/genopts.yml
  COMMAND
    Genopts::Python3_Interpreter -m genopts --config
    ${CMAKE_CURRENT_BINARY_DIR}/genopts.yml
    ${CMAKE_CURRENT_LIST_DIR}/file_options.yml
)
