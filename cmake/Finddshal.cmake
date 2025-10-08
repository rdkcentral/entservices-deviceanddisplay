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


# CMake module to find dsHal library and header
# This module defines the following variables:
#  DEVICE_HAL_FOUND - True if the library and header were found
#  DEVICE_HAL_INCLUDE_DIRS - Include directories for PowerHal
#  DEVICE_HAL_LIBRARIES - Libraries to link against

find_path(DEVICE_HAL_INCLUDE_DIR
  NAMES dsFPD.h
  PATH_SUFFIXES rdk/halif/ds-hal
)

find_library(DEVICE_HAL_LIBRARY
  NAMES ds-hal
)
find_library(DSHALSRV_LIBRARY NAMES dshalsrv)

if(DSHALSRV_LIBRARY)
  set(DSHALSRV_LIBRARIES ${DSHALSRV_LIBRARY})
  message("The value of DSHALSRV_LIBRARIES is: ${DSHALSRV_LIBRARIES}")
else()
  message(WARNING "Could not find dshalsrv")
endif()

# Set the results
if(DEVICE_HAL_INCLUDE_DIR AND DEVICE_HAL_LIBRARY)
  set(DEVICE_HAL_FOUND TRUE)
  set(DEVICE_HAL_INCLUDE_DIRS ${DEVICE_HAL_INCLUDE_DIR})
  set(DEVICE_HAL_LIBRARIES ${DEVICE_HAL_LIBRARY})
  message("The value of DEVICE_HAL_LIBRARY is: ${DEVICE_HAL_LIBRARIES}")
else()
  set(DEVICE_HAL_FOUND FALSE)
  message(WARNING "Could not find ds-hal")
endif()

# Provide user feedback
if(DEVICE_HAL_FOUND)
  message(STATUS "Found ds hal libs @ ${DEVICE_HAL_LIBRARIES}")
  message(STATUS "Found ds hal includes @ ${DEVICE_HAL_INCLUDE_DIRS}")
else()
  message(WARNING "Could not find dsFPD")
endif()

mark_as_advanced(DEVICE_HAL_INCLUDE_DIR DEVICE_HAL_LIBRARY)
