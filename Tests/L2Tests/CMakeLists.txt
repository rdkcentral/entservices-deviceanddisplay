# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
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


set(PLUGIN_NAME L2TestsDD)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(THUNDER_PORT 9998)

find_package(${NAMESPACE}Plugins REQUIRED)

if(PLUGIN_WAREHOUSE)
    set(SRC_FILES ${SRC_FILES} tests/Warehouse_L2Test.cpp)
endif()

if(PLUGIN_DEVICEDIAGNOSTICS)
    set(SRC_FILES ${SRC_FILES} tests/DeviceDiagnostics_L2Test.cpp)
endif()

if(PLUGIN_DISPLAYSETTINGS)
    set(SRC_FILES ${SRC_FILES} tests/DisplaySettings_L2Test.cpp)
endif()

if(PLUGIN_SYSTEMSERVICES)
    set(SRC_FILES ${SRC_FILES} tests/SystemService_L2Test.cpp)
endif()

if(PLUGIN_USERPREFERENCES)
    set(SRC_FILES ${SRC_FILES} tests/UserPreferences_L2Test.cpp) 
endif()

if(PLUGIN_POWERMANAGER)
    set(SRC_FILES ${SRC_FILES} tests/PowerManager_L2Test.cpp)
endif()

if(PLUGIN_FRAMERATE)
    set(SRC_FILES ${SRC_FILES} tests/FrameRate_L2Test.cpp)
endif()

add_library(${MODULE_NAME} SHARED ${SRC_FILES})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES)

target_compile_definitions(${MODULE_NAME}
            PRIVATE
            MODULE_NAME=Plugin_${PLUGIN_NAME}
            THUNDER_PORT="${THUNDER_PORT}")

target_compile_options(${MODULE_NAME} PRIVATE -Wno-error)
target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

if (NOT L2_TEST_OOP_RPC)
    find_library(TESTMOCKLIB_LIBRARIES NAMES TestMocklib)
    if (TESTMOCKLIB_LIBRARIES)
        message ("Found mock library - ${TESTMOCKLIB_LIBRARIES}")
        target_link_libraries(${MODULE_NAME} PRIVATE ${TESTMOCKLIB_LIBRARIES})
    else (TESTMOCKLIB_LIBRARIES)
         message ("Require ${TESTMOCKLIB_LIBRARIES} library")
    endif (TESTMOCKLIB_LIBRARIES)
endif (NOT L2_TEST_OOP_RPC)

find_library(MOCKACCESSOR_LIBRARIES NAMES MockAccessor)
if (MOCKACCESSOR_LIBRARIES)
    message ("Found MockAccessor library - ${MOCKACCESSOR_LIBRARIES}")
    target_link_libraries(${MODULE_NAME} PRIVATE ${MOCKACCESSOR_LIBRARIES})
else (MOCKACCESSOR_LIBRARIES)
     message ("Require ${MOCKACCESSOR_LIBRARIES} library")
endif (MOCKACCESSOR_LIBRARIES)

target_include_directories(
    ${MODULE_NAME} PRIVATE ./
    ../../helpers
    ../../../entservices-testframework/Tests/mocks
    ../../../entservices-testframework/Tests/mocks/thunder
    ../../../entservices-testframework/Tests/mocks/devicesettings
    ../../../entservices-testframework/Tests/mocks/MockPlugin
    ../../../entservices-testframework/Tests/L2Tests/L2TestsPlugin
    ${CMAKE_INSTALL_PREFIX}/include
    )

install(TARGETS ${MODULE_NAME} DESTINATION lib)
