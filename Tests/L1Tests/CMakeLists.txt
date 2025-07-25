# If not stated otherwise in this file or this component's LICENSE file the
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


cmake_minimum_required(VERSION 3.8)
set(PLUGIN_NAME L1TestsDD)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(${NAMESPACE}Plugins REQUIRED)

set (TEST_SRC
    tests/test_UtilsFile.cpp
)

set (TEST_LIB
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
)

set (TEST_INC ../../helpers)

#########################################################################################
# add_plugin_test_ex: Macro to add plugin tests, it will append to TEST_SRC, TEST_INC,
#                     and TEST_LIB. Args are positional.
#                     arg1: test file list as **string**
#                     arg2: include dir list as **string**
#                     arg3: plugin libs to link list as **string**
#
# sample invocation :
#    add_plugin_test_ex(PLUGIN_NAME
#                       "test/file1.cpp;test/file2.cpp"
#                       "../../inc_dir1;../../inc_dir2"
#                       "${NAMESPACE}PluginName;${NAMESPACE}PluginNameImplementation")
# ----------------------------------------- OR ------------------------------------------
# list(APPEND PLUGIN_NAME_SRC test/file1.cpp)
# list(APPEND PLUGIN_NAME_SRC test/file2.cpp)
#
# list(APPEND PLUGIN_NAME_INC ../../inc_dir1)
# list(APPEND PLUGIN_NAME_INC ../../inc_dir2)
#
# list(APPEND PLUGIN_NAME_LIB ${NAMESPACE}PluginName)
# list(APPEND PLUGIN_NAME_LIB ${NAMESPACE}PluginNameImplementation)
#
#    add_plugin_test_ex(PLUGIN_NAME
#                       "${PLUGIN_NAME_SRC}"
#                       "${PLUGIN_NAME_INC}"
#                       "${PLUGIN_NAME_LIB}")
#
# NOTE: Alternatively test can choose to update `TEST_SRC`, `TEST_INC` & `TEST_LIB`
#       directly (like in the case of Miracast)
#########################################################################################
macro(add_plugin_test_ex plugin_opt plugin_test_sources_str plugin_includes_str plugin_libs_str)
    # Check if the plugin option is enabled
    if(${plugin_opt})
        message(STATUS "${plugin_opt}=ON")

        string(REPLACE ";" ";" srclist "${plugin_test_sources_str}")
        string(REPLACE ";" ";" inclist "${plugin_includes_str}")
        string(REPLACE ";" ";" liblist "${plugin_libs_str}")

        foreach(item IN LISTS srclist)
            # Add each test source file
            list(APPEND TEST_SRC ${item})
        endforeach()

        foreach(item IN LISTS inclist)
            # Add each include directory
            list(APPEND TEST_INC ${item})
        endforeach()

        foreach(item IN LISTS liblist)
            # Add each libraries to link
            list(APPEND TEST_LIB ${item})
        endforeach()
    else()
        message(STATUS "${plugin_opt}=OFF")
    endif()
endmacro()

# helper to add plugin test
macro(add_plugin_test plugin_name test_files)
    # Convert plugin name to uppercase for the option variable
    string(TOUPPER "${plugin_name}" plugin_option)
    set(plugin_opt "PLUGIN_${plugin_option}")

    add_plugin_test_ex(${plugin_opt} "${test_files}" "../../${plugin_name}" "${NAMESPACE}${plugin_name}")
endmacro()

# PLUGIN_POWERMANAGER
#set (POWERMANAGER_INC ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/PowerManager ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/helpers)
#set (POWERMANAGER_LIBS ${NAMESPACE}PowerManager ${NAMESPACE}PowerManagerImplementation)
#add_plugin_test_ex(PLUGIN_POWERMANAGER ../mocks/PowerManagerMock.cpp "${POWERMANAGER_INC}" "${POWERMANAGER_LIBS}")

# PLUGIN_DEVICEINFO
#set (DEVICEINFO_INC ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/DeviceInfo ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/helpers)
#set (DEVICEINFO_SRC tests/test_DeviceAudioCapabilities.cpp tests/test_DeviceInfo.cpp tests/test_DeviceInfoJsonRpc.cpp tests/test_DeviceInfoWeb.cpp test_FirmwareVersion tests/test_DeviceVideoCapabilities.cpp)
#add_plugin_test_ex(PLUGIN_DEVICEINFO "${DEVICEINFO_SRC}" "${DEVICEINFO_INC}" "${NAMESPACE}DeviceInfo")

# PLUGIN_WAREHOUSE
set (WAREHOUSE_INC ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/Warehouse ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/helpers)
set (WAREHOUSE_LIBS ${NAMESPACE}Warehouse ${NAMESPACE}WarehouseImplementation)
add_plugin_test_ex(PLUGIN_WAREHOUSE tests/test_Warehouse.cpp "${WAREHOUSE_INC}" "${WAREHOUSE_LIBS}")


# PLUGIN_FRAMERATE
set (FRAMERATE_INC ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/FrameRate ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/helpers)
set (FRAMERATE_LIBS ${NAMESPACE}FrameRate ${NAMESPACE}FrameRateImplementation)
add_plugin_test_ex(PLUGIN_FRAMERATE tests/test_FrameRate.cpp "${FRAMERATE_INC}" "${FRAMERATE_LIBS}")


# PLUGIN_USERPREFERENCES
set (USERPREFERENCES_INC ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/UserPreferences ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/helpers)
add_plugin_test_ex(PLUGIN_USERPREFERENCES tests/test_UserPreferences.cpp "${USERPREFERENCES_INC}" "${NAMESPACE}UserPreferences")

# PLUGIN_SYSTEMSERVICES
#set (SYSTEMSERVICES_INC ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/SystemServices ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/helpers)
#add_plugin_test_ex(PLUGIN_SYSTEMSERVICES tests/test_SystemServices.cpp "${SYSTEMSERVICES_INC}" "${NAMESPACE}SystemServices")

# PLUGIN_DEVICEDIAGNOSTICS
set (DEVICEDIAGNOSTICS_INC ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/DeviceDiagnostics ${CMAKE_SOURCE_DIR}/../entservices-deviceanddisplay/helpers)
set (DEVICEDIAGNOSTICS_LIBS ${NAMESPACE}DeviceDiagnostics ${NAMESPACE}DeviceDiagnosticsImplementation)
add_plugin_test_ex(PLUGIN_DEVICEDIAGNOSTICS tests/test_DeviceDiagnostics.cpp "${DEVICEDIAGNOSTICS_INC}" "${DEVICEDIAGNOSTICS_LIBS}")

add_library(${MODULE_NAME} SHARED ${TEST_SRC})

set_source_files_properties(
        tests/test_DeviceAudioCapabilities.cpp
        tests/test_DeviceVideoCapabilities.cpp
        tests/test_SystemServices.cpp
        tests/test_Warehouse.cpp
        PROPERTIES COMPILE_FLAGS "-fexceptions")


include_directories(${TEST_INC})

target_link_directories(${MODULE_NAME} PUBLIC ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib/wpeframework/plugins)

target_link_libraries(${MODULE_NAME} ${TEST_LIB})

target_include_directories(${MODULE_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
        ${CMAKE_SOURCE_DIR}/../entservices-testframework/Tests/mocks
        ${CMAKE_SOURCE_DIR}/../entservices-testframework/Tests/mocks/devicesettings
        ${CMAKE_SOURCE_DIR}/../entservices-testframework/Tests/mocks/thunder
        ${CMAKE_SOURCE_DIR}/../Thunder/Source/plugins
        )

#install(TARGETS ${PROJECT_NAME} DESTINATION bin)

install(TARGETS ${MODULE_NAME} DESTINATION lib)
write_config(${PLUGIN_NAME})


