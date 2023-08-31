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

add_library(rpc_common OBJECT)
target_sources(
  rpc_common
  INTERFACE endpoint.hpp request.hpp serialization.hpp utilities.hpp
            signal_listener.hpp
  PRIVATE endpoint.cpp
)

target_link_libraries(rpc_common PUBLIC logger::logger thallium asio::asio)

# get the parent directory of the current directory so we can include
# headers from these libraries as `<net/*.hpp>`
get_filename_component(PARENT_DIRECTORY "../" ABSOLUTE)

add_library(rpc_client STATIC)
target_sources(
  rpc_client
  INTERFACE client.hpp
  PRIVATE client.cpp
)

target_link_libraries(rpc_client PUBLIC rpc_common)
target_include_directories(rpc_client PUBLIC ${PARENT_DIRECTORY})
set_property(TARGET rpc_client PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(net::rpc_client ALIAS rpc_client)

add_library(rpc_server STATIC)
target_sources(
  rpc_server
  INTERFACE server.hpp
  PRIVATE server.cpp
)

target_link_libraries(rpc_server PUBLIC rpc_common)
target_include_directories(rpc_server PUBLIC ${PARENT_DIRECTORY})
set_property(TARGET rpc_server PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(net::rpc_server ALIAS rpc_server)

add_library(net_net STATIC)
target_include_directories(net_net PUBLIC ${PARENT_DIRECTORY})
target_link_libraries(net_net PUBLIC rpc_client rpc_server)
set_property(TARGET net_net PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(net::net ALIAS net_net)
