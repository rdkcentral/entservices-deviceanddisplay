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


# CMake module to find PowerHal library and header
# This module defines the following variables:
#  POWER_HAL_FOUND - True if the library and header were found
#  POWER_HAL_INCLUDE_DIRS - Include directories for PowerHal
#  POWER_HAL_LIBRARIES - Libraries to link against

find_path(POWER_HAL_INCLUDE_DIR
  NAMES plat_power.h
  PATH_SUFFIXES rdk/halif/power-manager
)

find_library(POWER_HAL_LIBRARY
  NAMES iarmmgrs-power-hal
)

# Set the results
if(POWER_HAL_INCLUDE_DIR AND POWER_HAL_LIBRARY)
  set(POWER_HAL_FOUND TRUE)
  set(POWER_HAL_INCLUDE_DIRS ${POWER_HAL_INCLUDE_DIR})
  set(POWER_HAL_LIBRARIES ${POWER_HAL_LIBRARY})
else()
  set(POWER_HAL_FOUND FALSE)
endif()

# Provide user feedback
if(POWER_HAL_FOUND)
  message(STATUS "Found power hal libs @ ${POWER_HAL_LIBRARIES}")
  message(STATUS "Found power hal includes @ ${POWER_HAL_INCLUDE_DIRS}")
else()
  message(WARNING "Could not find plat_power")
endif()

mark_as_advanced(POWER_HAL_INCLUDE_DIR POWER_HAL_LIBRARY)
