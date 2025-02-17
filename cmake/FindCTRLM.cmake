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

# - Try to find ControlManager
# Once done this will define
#  CTRLM_FOUND - System has ControlManager
#  CTRLM_INCLUDE_DIRS - The ControlManager include directories
#

find_package(PkgConfig)

find_path(CTRLM_INCLUDE_DIRS NAMES ctrlm_ipc.h)

set(CTRLM_INCLUDE_DIRS ${CTRLM_INCLUDE_DIRS})
set(CTRLM_INCLUDE_DIRS ${CTRLM_INCLUDE_DIRS} CACHE PATH "Path to ControlManager include")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CTRLM DEFAULT_MSG CTRLM_INCLUDE_DIRS)

mark_as_advanced(
    CTRLM_FOUND
    CTRLM_INCLUDE_DIRS)
