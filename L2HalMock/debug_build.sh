#!/bin/bash

# Define ANSI color codes for green
GREEN='\033[0;32m'     # Green text
YELLOW='\033[0;33m'     # Yellow text
NC='\033[0m'           # No color (resets to default)

SCRIPT=$(readlink -f "$0")
SCRIPTS_DIR=`dirname "$SCRIPT"`
RDK_DIR=$SCRIPTS_DIR/../
WORKSPACE=$SCRIPTS_DIR/workspace

PS3='Please enter your choice: '
options=("Build Thunder tool" "Build Thunder" "Build rdkservices-apis" "Build IARM" "Build DeviceSettings" "Build HdmiCec" "Build rdkservices" "Build All" "Quit")
# set -x #enable debugging mode
select opt in "${options[@]}" 
do
    case $opt in
        "Build Thunder tool")
            rm -rf ThunderTools
            echo -e "${GREEN}========================================Building thunder tools===============================================${NC}"
            git clone -b R4_4 git@github.com:rdkcentral/ThunderTools.git
            echo -e "${GREEN}========================================Apply Patchs===============================================${NC}"
            cd ThunderTools
            git apply $SCRIPTS_DIR/patches/00010-R4.4-Add-support-for-project-dir.patch
            echo -e "${GREEN}========================================END Apply Patchs===============================================${NC}"
            cmake -G Ninja -S . -B ThunderTools/build -DCMAKE_INSTALL_PREFIX="install"
            cmake --build ThunderTools/build --target install
            ;;

        "Build Thunder")
            # cd $WORKSPACE
            # echo -e "${GREEN}========================================Building Thunder===============================================${NC}"
            rm -rf Thunder
            git clone -b R4_4 git@github.com:rdkcentral/Thunder.git 
            echo -e "${GREEN}======================================== Apply Patchs===============================================${NC}"
            cd Thunder
            # git apply $SCRIPTS_DIR/patches/1004-Add-support-for-project-dir.patch
            echo -e "${GREEN}========================================END Apply Patchs===============================================${NC}"

            cmake -G Ninja -S . -B build -DBINDING="127.0.0.1" -DBUILD_TYPE="Debug" -DCMAKE_INSTALL_PREFIX="$WORKSPACE/install/usr" -DCMAKE_MODULE_PATH="${WORKSPACE}/install/usr/include/WPEFramework/Modules" -DPORT="55555" -DPROXYSTUB_PATH="${WORKSPACE}/install/usr/lib/wpeframework/proxystubs" -DGENERIC_CMAKE_MODULE_PATH="${WORKSPACE}/install/usr/lib/wpeframework/plugins" -DVOLATILE_PATH="tmp" &&
            cmake --build build -j8 &&
            cmake --install build          
            ;;

        "Build rdkservices-apis")
            # source ./env.sh
            # cd $WORKSPACE
            # echo -e "${GREEN}========================================Building Thunder===============================================${NC}"
            rm -rf entservices-apis
            git clone git@github.com:rdkcentral/entservices-apis.git
            cd entservices-apis
            # cmake -G Ninja -S . -B build \
            # -DCMAKE_INSTALL_PREFIX="install" \
            # -DCMAKE_BUILD_TYPE="Debug" \
            # -DTOOLS_SYSROOT="${PWD}"
            cmake -S . -B build/entservices-apis -DEXCEPTIONS_ENABLE=OFF -DCMAKE_INSTALL_PREFIX="$WORKSPACE/install/usr" -DCMAKE_MODULE_PATH="/home/administrator/PROJECT/New_Sink/New_patch/DS/222/Iarm_dev_banch/PowerManager3/entservices-deviceanddisplay/L2HalMock/ThunderTools/install/include/WPEFramework/Modules" &&
            # cmake --build build --target install
            cmake --build build/entservices-apis -j8 &&
            cmake --install build/entservices-apis
            ;;
            # # cmake -S . -B build/entservices-apis -DEXCEPTIONS_ENABLE=OFF -DCMAKE_INSTALL_PREFIX="$WORKSPACE/install/usr" -DCMAKE_MODULE_PATH="$WORKSPACE/install/tools/cmake" &&
            # # cmake -S . -B build/entservic es-apis -DEXCEPTIONS_ENABLE=ON -DCMAKE_BUILD_TYPE="Debug" -DCMAKE_INSTALL_PREFIX="$WORKSPACE/install/usr" -DCMAKE_MODULE_PATH="${WORKSPACE}/install/usr/include/WPEFramework/Modules" -DPORT="55555" -DPROXYSTUB_PATH="${WORKSPACE}/install/usr/lib/wpeframework/proxystubs" -DSYSTEM_PATH="${WORKSPACE}/install/usr/lib/wpeframework/plugins" -DVOLATILE_PATH="tmp"
            # cmake --build build -j8 &&
            # cmake --install build
            # cd entservices-apis
            # cmake -G Ninja -S . -B build \
            # -DCMAKE_INSTALL_PREFIX="install" \
            # -DTOOLS_SYSROOT="${PWD}"
            # cmake --build build --target install
            

        "Build IARM")
            echo "you chose choice $opt"
            # Build IARM
            echo -e "${GREEN}========================================Build IARM===============================================${NC}"
            (cd $WORKSPACE/deps/rdk/iarmbus/ && ./build.sh)
            ;;
        "Build DeviceSettings")
            echo "you chose choice $opt"
            echo -e "${GREEN}========================================Build DeviceSettings===============================================${NC}"
            # Build DeviceSettings
            (cd $WORKSPACE/deps/rdk/devicesettings/ && ./build.sh)
            ;;

        "Build HdmiCec")
            echo "you chose choice $opt"
            echo -e "${GREEN}========================================Build HdmiCec===============================================${NC}"
            # Build HdmiCec
            (cd $WORKSPACE/deps/rdk/hdmicec/ && ./build.sh)
            ;;
        
        "Build rdkservices")
            echo "you chose choice $opt"
            echo -e "${GREEN}========================================Build rdkservices===============================================${NC}"
            source ./env.sh
            cd $RDK_DIR;
            cmake -S . -B build \
            -DCMAKE_INSTALL_PREFIX="$WORKSPACE/install/usr" \
            -DCMAKE_MODULE_PATH="$WORKSPACE/install/usr/include/WPEFramework/Modules" \
            -DPLUGIN_POWERMANAGER=ON \
            -DUSE_THUNDER_R4=ON \
            -DCOMCAST_CONFIG=OFF \
            -DCEC_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/ccec/include" \
            -DOSAL_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/osal/include" \
            -DCEC_HOST_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/host/include" \
            -DDS_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/install/lib/libds.so" \
            -DDS_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/ds/include" \
            -DDSHAL_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/hal/include" \
            -DDSRPC_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/rpc/include" \
            -DIARMBUS_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/iarmbus/core/include" \
            -DIARMRECEIVER_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/iarmmgrs/receiver/include" \
            -DIARMPWR_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/iarmmgrs/hal/include" \
            -DCEC_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/install/lib/libRCEC.so" \
            -DIARMBUS_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/iarmbus/install/libIARMBus.so" \
            -DDSHAL_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/devicesettings/install/lib/libdshalcli.so" \
            -DCEC_HAL_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/install/lib/libRCECHal.so" \
            -DOSAL_LIBRARIES="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/install/lib/libRCECOSHal.so" \
            -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -Wall -Werror -Wno-error=format=-Wl,-wrap,system -Wl,-wrap,popen -Wl,-wrap,syslog"

            (cd $RDK_DIR && cmake --build build --target install)
            ;;

        "Build All")
            echo "you chose choice $opt"
            echo -e "${GREEN}========================================Building All===============================================${NC}"
            ./build.sh
            ;;

        "Quit")
            break
            ;;
        *) echo "invalid option $REPLY";;
    esac
    echo -e "${YELLOW}1) Build IARM 2) Build DeviceSettings 3) Build HdmiCec 4) Build rdkservices 5) Build All 6) Quit${NC}"
done

set +x #disable debugging mode
