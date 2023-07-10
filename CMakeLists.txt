#
# Copyright (c) 2019-2020, University of Southampton and Contributors.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.12)

include(cmake/Utilities.cmake)

option(INSTALL_SYSTEMC "Download, build & install SystemC" OFF)
option(INSTALL_SYSTEMCAMS "Download, build & install SystemC-AMS" OFF)
option(INSTALL_DEPENDENCIES "Download, build & install other dependencies only" OFF)
option(ENABLE_TESTS "Build tests" ON)

set(EP_INSTALL_DIR $ENV{HOME}/.local CACHE STRING
                "Installation directory for dependencies")

IF(INSTALL_DEPENDENCIES)
  project(INSTALL_DEPENDENCIES NONE)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  include(cmake/InstallDependencies.cmake)
  return()
ELSE()
  project(fused-ps)
ENDIF()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Enable  sanitizer on debug build
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE}
#       -march=native -fsanitize=address,undefined,leak")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}
  -Wall -Wextra -Wno-reorder -Wno-unused -Wno-sign-compare")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_EXE_LINKER_FLAGS -static)

LIST(APPEND CMAKE_PREFIX_PATH ${EP_INSTALL_DIR})
find_package(spdlog REQUIRED )
find_package(yaml-cpp REQUIRED)

include_directories(.)
link_directories(${EP_INSTALL_DIR}/lib)
include_directories(${EP_INSTALL_DIR}/include)

add_subdirectory(ps)
add_subdirectory(examples)

IF(ENABLE_TESTS)
  enable_testing()

  # Extra command to check outputs of failing tests
  add_custom_target(testcheck COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)

  # ---- "unit-tests" ------
  add_subdirectory(test)
  add_test(NAME PowerModelChannel COMMAND testPowerModelChannel)
ENDIF()
