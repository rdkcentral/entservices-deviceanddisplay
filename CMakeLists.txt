###
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2023 RDK Management
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
###

cmake_minimum_required(VERSION 3.3)

find_package(WPEFramework)

# All packages that did not deliver a CMake Find script (and some deprecated scripts that need to be removed)
# are located in the cmake directory. Include it in the search.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# This configuration enables generation of Test binaries which can be used to test respective plugins.
option(TESTBINARIES "Generate plugin specific test binaries" OFF)
if(TESTBINARIES)
    message(INFO "Enabling test binaries")
    if(PLUGIN_FRAMERATE)
        set(FRAMERATETESTAPP ON CACHE INTERNAL "Enable FrameRate test application")
    endif()
    add_subdirectory(Tests)
endif()

option(COMCAST_CONFIG "Comcast services configuration" ON)
if(COMCAST_CONFIG)
    include(services.cmake)
endif()

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGE_DIRECTORY)

# for writing pc and config files
include(CmakeHelperFunctions)

if(RDK_SERVICE_L2_TEST)
    add_subdirectory(Tests/L2Tests)
endif()

if(RDK_SERVICES_L1_TEST)
    add_subdirectory(Tests/L1Tests)
endif()

if(PLUGIN_DISPLAYSETTINGS)
    add_subdirectory(DisplaySettings)
endif()

if(PLUGIN_SYSTEMSERVICES)
    add_subdirectory(SystemServices)
endif()

if(PLUGIN_POWERMANAGER)
    add_subdirectory(PowerManager)
endif()

if(PLUGIN_USERPREFERENCES)
    add_subdirectory(UserPreferences)
endif()

if(PLUGIN_FRAMERATE)
    add_subdirectory(FrameRate)
endif()

if(PLUGIN_WAREHOUSE)
    add_subdirectory(Warehouse)
endif()

if(PLUGIN_DEVICEINFO)
    add_subdirectory(DeviceInfo)
endif()

if(PLUGIN_DISPLAYINFO)
    add_subdirectory(DisplayInfo)
endif()

if(PLUGIN_DEVICEDIAGNOSTICS)
    add_subdirectory(DeviceDiagnostics)
endif()

if(PLUGIN_SYSTEMMODE)
    add_subdirectory(SystemMode)
endif()

if(WPEFRAMEWORK_CREATE_IPKG_TARGETS)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEB_COMPONENT_INSTALL ON)
    set(CPACK_COMPONENTS_GROUPING IGNORE)

    set(CPACK_DEBIAN_PACKAGE_NAME "${WPEFRAMEWORK_PLUGINS_OPKG_NAME}")
    set(CPACK_DEBIAN_PACKAGE_VERSION "${WPEFRAMEWORK_PLUGINS_OPKG_VERSION}")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${WPEFRAMEWORK_PLUGINS_OPKG_ARCHITECTURE}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${WPEFRAMEWORK_PLUGINS_OPKG_MAINTAINER}")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${WPEFRAMEWORK_PLUGINS_OPKG_DESCRIPTION}")
    set(CPACK_PACKAGE_FILE_NAME "${WPEFRAMEWORK_PLUGINS_OPKG_FILE_NAME}")

    include(CPack)
endif()
