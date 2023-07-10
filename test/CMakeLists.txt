#
# Copyright (c) 2019-2020, University of Southampton and Contributors.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

add_executable(testPowerModelChannel
  test_PowerModelChannel.cpp
  )

target_link_libraries(testPowerModelChannel
  PRIVATE
    systemc
    PowerSystem
    spdlog::spdlog
    )
