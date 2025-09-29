# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

set(PLUGIN_NAME DeviceSettingsManager)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Imp)

set(PLUGIN_DEVICESETTINGS_AUTOSTART "true" CACHE STRING "Automatically start DeviceSettingsManager plugin")
set(PLUGIN_DEVICESETTINGS_STARTUPORDER "30" CACHE STRING "To configure startup order of DeviceSettingsManager plugin")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../wpeframework-clientlibraries/4.4.2-r20/build/Source/devicesettings")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_library(PROCPS_LIBRARIES NAMES procps)

# Set WPEFrameworkDSController library path directly
set(WPEFRAMEWORK_DSCONTROLLER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../wpeframework-clientlibraries/4.4.2-r20/build/Source/devicesettings")
set(WPEFRAMEWORK_DSCONTROLLER_LIB "${WPEFRAMEWORK_DSCONTROLLER_PATH}/libWPEFrameworkDSController.so")

# Check if the library exists
if(NOT EXISTS ${WPEFRAMEWORK_DSCONTROLLER_LIB})
    message(FATAL_ERROR "WPEFrameworkDSController library not found at: ${WPEFRAMEWORK_DSCONTROLLER_LIB}")
else()
    message(STATUS "Found WPEFrameworkDSController library at: ${WPEFRAMEWORK_DSCONTROLLER_LIB}")
endif()

#find_package(DeepSleepHal)
#find_package(MFR)
#find_package(RFC)

#add_library(${MODULE_NAME} SHARED
#    Module.cpp
#    DeviceSettingsManager.cpp)

add_executable(${MODULE_NAME}
    Module.cpp
    DeviceSettingsManager.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

#add_library(${PLUGIN_IMPLEMENTATION} SHARED
#    Module.cpp
#    DeviceSettingsManagerImp.cpp
#    fpd.cpp
#    HdmiIn.cpp
#)

add_executable(${PLUGIN_IMPLEMENTATION}
    Module.cpp
    DeviceSettingsManagerImp.cpp
    fpd.cpp
    HdmiIn.cpp
)

find_package(dshal)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../helpers
)

set_target_properties(${PLUGIN_IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

#if(RDK_SERVICES_L1_TEST OR RDK_SERVICE_L2_TEST)
#
#    target_compile_definitions(${PLUGIN_IMPLEMENTATION}
#                               PUBLIC
#                                 PLATCO_BOOTTO_STANDBY
#                                 ENABLE_THERMAL_PROTECTION
#                                 OFFLINE_MAINT_REBOOT)
#
#   find_library(TESTMOCKLIB_LIBRARIES NAMES TestMocklib)
#   if (TESTMOCKLIB_LIBRARIES)
#       message ("linking mock libraries ${TESTMOCKLIB_LIBRARIES} library")
#       target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${TESTMOCKLIB_LIBRARIES})
#   else (TESTMOCKLIB_LIBRARIES)
#       message ("Require ${TESTMOCKLIB_LIBRARIES} library")
#   endif ()
#endif ()

if(PROCPS_LIBRARIES)
    target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${PROCPS_LIBRARIES})
endif()

if (MFR_FOUND)
  target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${MFR_LIBRARIES})
  target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${MFR_INCLUDE_DIRS})
endif()

find_package(DS)
if (DS_FOUND)
    find_package(IARMBus)
    add_definitions(-DDS_FOUND)
    target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${IARMBUS_INCLUDE_DIRS})
    target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${DS_INCLUDE_DIRS})
    target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${IARMBUS_LIBRARIES} ${DS_LIBRARIES})
else (DS_FOUND)
    target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins)
endif(DS_FOUND)

find_path(DEVICE_HAL_INCLUDE_DIR
  NAMES dsFPD.h
  PATH_SUFFIXES rdk/halif/ds-hal
)

find_library(DEVICE_RFC_LIBRARY
  NAMES ds-hal
)

set(DEVICE_HAL_INCLUDE_DIRS ${DEVICE_HAL_INCLUDE_DIR})
set(DEVICE_RFC_LIBRARIES ${DEVICE_RFC_LIBRARY})


#find_library(DSHAL_LIBRARIES NAMES dshalsrv)
#set(DS_LIBRARIES ${DS_LIBRARIES} ${DSHAL_LIBRARIES})

target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${DEVICE_HAL_INCLUDE_DIRS})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${DEVICE_HAL_LIBRARIES})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${DSHALSRV_LIBRARIES})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${OEMHAL_LIBRARIES})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${IARMBUS_LIBRARIES})
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${DEVICE_RFC_LIBRARIES})

#if(DEEPSLEEP_HAL_FOUND)
#    target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${DEEPSLEEP_HAL_INCLUDE_DIRS})
#    target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${DEEPSLEEP_HAL_LIBRARIES})
#endif()

#if (RFC_FOUND)
#  target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${RFC_LIBRARIES})
#  target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${RFC_INCLUDE_DIRS})
#endif()

target_link_libraries(${PLUGIN_IMPLEMENTATION}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${PLUGIN_IMPLEMENTATION}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})

add_executable (DSApp main.cpp)

# Find CURL library
find_package(CURL REQUIRED)

target_link_libraries (DSApp
    PRIVATE
        ${WPEFRAMEWORK_DSCONTROLLER_LIB}
        ${CURL_LIBRARIES}
        pthread)
