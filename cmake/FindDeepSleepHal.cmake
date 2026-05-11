# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# CMake module to find DeepSleepHal library and header
# This module defines the following variables:
#  DEEPSLEEP_HAL_FOUND - True if the library and header were found
#  DEEPSLEEP_HAL_INCLUDE_DIRS - Include directories for DeepSleepHal
#  DEEPSLEEP_HAL_LIBRARIES - Libraries to link against

find_path(DEEPSLEEP_HAL_INCLUDE_DIR
  NAMES deepSleepMgr.h
  PATH_SUFFIXES rdk/halif/deepsleep-manager
)

find_library(DEEPSLEEP_HAL_LIBRARY
  NAMES iarmmgrs-deepsleep-hal
)

# Set the results
if(DEEPSLEEP_HAL_INCLUDE_DIR AND DEEPSLEEP_HAL_LIBRARY)
  set(DEEPSLEEP_HAL_FOUND TRUE)
  set(DEEPSLEEP_HAL_INCLUDE_DIRS ${DEEPSLEEP_HAL_INCLUDE_DIR})
  set(DEEPSLEEP_HAL_LIBRARIES ${DEEPSLEEP_HAL_LIBRARY})
else()
  set(DEEPSLEEP_HAL_FOUND FALSE)
endif()

# Provide user feedback
if(DEEPSLEEP_HAL_FOUND)
  message(STATUS "Found Deepsleep hal libs @ ${DEEPSLEEP_HAL_LIBRARIES}")
  message(STATUS "Found DeepSleepHal hal includes @ ${DEEPSLEEP_HAL_INCLUDE_DIR}")
else()
  message(WARNING "Could not find DeepSleepHal")
endif()

mark_as_advanced(DEEPSLEEP_HAL_INCLUDE_DIR DEEPSLEEP_HAL_LIBRARY)
