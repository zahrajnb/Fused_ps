#
# Copyright (c) 2019-2020, University of Southampton and Contributors.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

include (ExternalProject)

set_property (DIRECTORY PROPERTY EP_BASE Dependencies)

if (INSTALL_SYSTEMC)
  # SystemC
  ExternalProject_Add (ep_systemc
    GIT_REPOSITORY https://github.com/accellera-official/systemc.git
    GIT_TAG 38b8a2c61aafe40de44296bee0c9c28ac4e80b01
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EP_INSTALL_DIR}
               -DCMAKE_CXX_STANDARD=11
    )
endif()

# spdlog
ExternalProject_Add (ep_spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG cf6f1dd01e660d5865d68bf5fa78f6376b89470a
  GIT_SHALLOW ON
  GIT_PROGRESS ON
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EP_INSTALL_DIR}
             -DSPDLOG_BUILD_TESTS=OFF
             -DSPDLOG_BUILD_EXAMPLE=OFF
  )

# yaml-cpp
ExternalProject_Add (ep_yaml_cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG 9a3624205e8774953ef18f57067b3426c1c5ada6
  GIT_SHALLOW ON
  GIT_PROGRESS ON
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EP_INSTALL_DIR}
             -DYAML_CPP_BUILD_TESTS=OFF
 )
