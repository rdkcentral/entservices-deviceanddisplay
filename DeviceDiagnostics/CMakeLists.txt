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

set(PLUGIN_NAME DeviceDiagnostics)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Implementation)

set(PLUGIN_DEVICEDIAGNOSTICS_STARTUPORDER "" CACHE STRING "To configure startup order of DeviceDiagnostics plugin")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
        DeviceDiagnostics.cpp
        Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_include_directories(${MODULE_NAME} PRIVATE ../helpers)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

add_library(${PLUGIN_IMPLEMENTATION} SHARED
        DeviceDiagnosticsImplementation.cpp
        Module.cpp)

set_target_properties(${PLUGIN_IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

if (RDK_SERVICE_L2_TEST)
   find_library(TESTMOCKLIB_LIBRARIES NAMES TestMocklib)
   if (TESTMOCKLIB_LIBRARIES)
       message ("linking mock libraries ${TESTMOCKLIB_LIBRARIES} library")
       target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${TESTMOCKLIB_LIBRARIES})
   else (TESTMOCKLIB_LIBRARIES)
       message ("Require ${TESTMOCKLIB_LIBRARIES} library")
   endif (TESTMOCKLIB_LIBRARIES)
endif (RDK_SERVICES_L2_TEST)

find_package(Curl)

target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${IARMBUS_INCLUDE_DIRS} ../helpers)

target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins CompileSettingsDebug::CompileSettingsDebug ${CURL_LIBRARY})

if(BUILD_ENABLE_ERM)
    target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE essosrmgr)
endif()

install(TARGETS ${PLUGIN_IMPLEMENTATION}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
