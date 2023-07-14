#
# Copyright (c) 2019-2020, University of Southampton and Contributors.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.12)

option(INSTALL_SYSTEMC "Download, build & install SystemC" OFF)
option(INSTALL_DEPENDENCIES "Download, build & install other dependencies only" OFF)

set(EP_INSTALL_DIR ${CMAKE_CURRENT_LIST_DIR}/imported CACHE STRING
								"Installation directory for dependencies")

IF(INSTALL_DEPENDENCIES)
	project(INSTALL_DEPENDENCIES NONE)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	include(cmake/InstallDependencies.cmake)
	return()
ELSE()
	project(Fused_ps VERSION 1.0.0 DESCRIPTION "fused ...")
ENDIF()

#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#Enable  sanitizer on debug build
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE}
#       -march=native -fsanitize=address,undefined,leak")
#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}
#	-Wall -Wextra -Wno-reorder -Wno-unused -Wno-sign-compare")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_EXE_LINKER_FLAGS -static)

#LIST(APPEND CMAKE_PREFIX_PATH ${EP_INSTALL_DIR}/lib/cmake/spdlog ${EP_INSTALL_DIR}/lib/cmake/SystemCLanguage ${EP_INSTALL_DIR}/lib/cmake/yaml-cpp)
LIST(APPEND CMAKE_PREFIX_PATH ${EP_INSTALL_DIR})

find_package(spdlog REQUIRED )
find_package(yaml-cpp REQUIRED)
find_package(SystemCLanguage REQUIRED)

include_directories(.)
link_directories(${EP_INSTALL_DIR}/lib)
include_directories(${EP_INSTALL_DIR}/include)

file (GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/ps/*.h")
file (GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/ps/*.cpp")
add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC SystemC::systemc spdlog::spdlog)