#!/bin/bash

# Define ANSI color codes
GREEN='\033[0;32m'     # Green text
YELLOW='\033[1;33m'    # Yellow text
NC='\033[0m'           # No color (reset)

set -x
# Set up directories
SCRIPT=$(readlink -f "$0")
SCRIPTS_DIR=$(dirname "$SCRIPT")
RDK_DIR="$SCRIPTS_DIR/../"
WORKSPACE="$SCRIPTS_DIR/workspace"
INCLUDE_DIR="/usr/include"
sudo rm -rf "$WORKSPACE"
mkdir "$WORKSPACE"

cd "$SCRIPTS_DIR"
rm -rf meta-rdk-video
git clone git@github.com:rdkcentral/meta-rdk-video.git


echo -e "${GREEN}========================================Clone Thunder tools===============================================${NC}"
rm -rf ThunderTools
rm -rf Thunder
rm -rf ThunderClientLibraries
rm -rf entservices-apis
BB_FILE="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework-tools_4.4.bb"
SRCREV=$(grep "^SRCREV" "$BB_FILE" | cut -d'"' -f2)

if [ -n "$SRCREV" ]; then
    echo "Found SRCREV: $SRCREV"
    # You can now use $SRCREV in git checkout or other commands
    git clone git@github.com:rdkcentral/ThunderTools.git && cd ThunderTools && git checkout R4_4 && git checkout "$SRCREV" || { echo "❌ Git clone failed."; exit 1; }
else
    echo "SRCREV not found in $BB_FILE"
    exit 1
fi


echo -e "${YELLOW}========================================Patch Apply Thunder tools===============================================${NC}"
# Path to the file containing SRC_URI entries
input_file="$BB_FILE"

# Base directory where patches are located
patch_base_dir="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework-tools/"

# Extract all .patch entries from multi-line SRC_URI blocks

patches=$(awk '
    BEGIN { in_src_uri = 0 }
    /SRC_URI/ { in_src_uri = 1 }
    in_src_uri {
        for (i = 1; i <= NF; i++) {
            if ($i ~ /file:\/\/.*\.patch/) {
                match($i, /file:\/\/[^ ]*\.patch/)
                print substr($i, RSTART, RLENGTH)
            }
        }
        if ($0 !~ /\\$/) {
            in_src_uri = 0
        }
    }
' "$input_file")

cd "$SCRIPTS_DIR/ThunderTools"
echo "$patches" | while read -r patch_uri; do
    patch_file="${patch_uri#file://}"
    full_path="$patch_base_dir/$patch_file"

    if [ -f "$full_path" ]; then
        echo "Applying patch: $full_path"
        git apply "$full_path"
        echo "✅ Successfully applied: $full_path"
    else
        echo "⚠️ Failed to apply: $full_path — skipping"
    fi
done

cd $SCRIPTS_DIR/ThunderTools/cmake
mv FindProxyStubGenerator.cmake.in. FindProxyStubGenerator.cmake.in

echo -e "${YELLOW}========================================Build Thunder tools===============================================${NC}"
cd $SCRIPTS_DIR
cmake -G Ninja -S ThunderTools -B ThunderTools/build -DCMAKE_INSTALL_PREFIX="${WORKSPACE}/install/usr"
# cmake --build build --target install
sudo ninja -C ThunderTools/build install

echo -e "${GREEN}======================================== Clone Thunder ===============================================${NC}"

sudo rm -rf Thunder
BB_FILE="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework_4.4.bb"
SRCREV_thunder=$(grep "^SRCREV_thunder" "$BB_FILE" | cut -d'"' -f2)

if [ -n "$SRCREV_thunder" ]; then
    echo "Found SRCREV: $SRCREV_thunder"
    git clone git@github.com:rdkcentral/Thunder.git && \
    cd Thunder && \
    git checkout R4_4 && \
    git checkout "$SRCREV_thunder" || { echo "❌ Git clone failed."; exit 1; }
else
    echo "SRCREV_thunder not found in $BB_FILE"
    exit 1
fi

# Apply patches
echo -e "${YELLOW}======================================== Apply Patches ===============================================${NC}"

# Path to the file containing SRC_URI entries
input_file="$BB_FILE"

# Base directory where patches are located
patch_base_dir="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework"

cd $SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework/r4.4/
mv 1004-Add-support-for-project-dir.patch 1004-Add-support-for-project-dir._patch

# Extract all .patch entries from multi-line SRC_URI blocks
patches=$(awk '
    BEGIN { in_src_uri = 0 }
    /SRC_URI/ { in_src_uri = 1 }
    in_src_uri {
        for (i = 1; i <= NF; i++) {
            if ($i ~ /file:\/\/.*\.patch/) {
                match($i, /file:\/\/[^ ]*\.patch/)
                print substr($i, RSTART, RLENGTH)
            }
        }
        if ($0 !~ /\\$/) {
            in_src_uri = 0
        }
    }
' "$input_file")

cd "$SCRIPTS_DIR/Thunder"
echo "$patches" | while read -r patch_uri; do
    patch_file="${patch_uri#file://}"
    full_path="$patch_base_dir/$patch_file"

    if [ -f "$full_path" ]; then
        echo "Applying patch: $full_path"
        git apply "$full_path"
        echo "✅ Successfully applied: $full_path"
    else
        echo "⚠️ Failed to apply: $full_path — skipping"
    fi
done

echo -e "${YELLOW}========================================Build Thunder===============================================${NC}"
cd $SCRIPTS_DIR

# Set your module name
MODULE_NAME="L2HalMock"

cmake -G Ninja -S Thunder -B Thunder/build \
            -DMODULE_NAME=ON \
            -DCMAKE_CXX_FLAGS="-DMODULE_NAME=\"${MODULE_NAME}\"" \
            -DBINDING="127.0.0.1" \
            -DCMAKE_BUILD_TYPE="Debug" \
            -DCMAKE_INSTALL_PREFIX="${WORKSPACE}/install/usr" \
            -DCMAKE_MODULE_PATH="${WORKSPACE}/install/usr/include/WPEFramework/Modules" \
            -DPORT="55555" \
            -DTOOLS_SYSROOT="${PWD}" \
            -DCMAKE_CURRENT_SOURCE_DIR="${SCRIPTS_DIR}" \
            -DINITV_SCRIPT=OFF
# cmake --build Thunder/build --target install
sudo ninja -C Thunder/build install
sudo chown -R $USER:$USER $WORKSPACE/install

cd $SCRIPTS_DIR
echo -e "${GREEN}========================================Clone ThunderClientLibrary===============================================${NC}"
BB_FILE="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework-clientlibraries_4.4.bb"
SRCREV_wpeframework=$(grep "^SRCREV_wpeframework-clientlibraries" "$BB_FILE" | cut -d'"' -f2)

if [ -n "$SRCREV_wpeframework" ]; then
    echo "Found SRCREV: $SRCREV_wpeframework"
    # You can now use $SRCREV_ThunderClientLibraries in git checkout or other commands
    git clone git@github.com:rdkcentral/ThunderClientLibraries.git && cd ThunderClientLibraries && git checkout R4_4 && git checkout "$SRCREV_wpeframework" || { echo "❌ Git clone failed."; exit 1; }
else
    echo "SRCREV_thunder not found in $BB_FILE"
    exit 1
fi

# Path to the file containing SRC_URI entries
input_file="$BB_FILE"

# Base directory where patches are located
patch_base_dir="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/wpeframework-clientlibraries"

# Extract all .patch entries from multi-line SRC_URI blocks
patches=$(awk '
    BEGIN { in_src_uri = 0 }
    /SRC_URI/ { in_src_uri = 1 }
    in_src_uri {
        for (i = 1; i <= NF; i++) {
            if ($i ~ /file:\/\/.*\.patch/) {
                match($i, /file:\/\/[^ ]*\.patch/)
                print substr($i, RSTART, RLENGTH)
            }
        }
        if ($0 !~ /\\$/) {
            in_src_uri = 0
        }
    }
' "$input_file")

cd "$SCRIPTS_DIR/ThunderClientLibraries"
echo "$patches" | while read -r patch_uri; do
    patch_file="${patch_uri#file://}"
    full_path="$patch_base_dir/$patch_file"

    if [ -f "$full_path" ]; then
        echo "Applying patch: $full_path"
        git apply "$full_path"
        echo "✅ Successfully applied: $full_path"
    else
        echo "⚠️ Failed to apply: $full_path — skipping"
    fi
done

echo -e "${GREEN}========================================Clone entservices-apis===============================================${NC}"
cd $SCRIPTS_DIR
sudo rm -rf entservices-apis

BB_FILE="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/entservices-apis.bb"
SRCREV_entservicesapis=$(grep "^SRCREV_entservices-apis" "$BB_FILE" | cut -d'"' -f2)

if [ -n "$SRCREV_entservicesapis" ]; then
    echo "Found SRCREV: $SRCREV_entservicesapis"
    # You can now use $SRCREV_entservicesapis in git checkout or other commands
    git clone git@github.com:rdkcentral/entservices-apis.git && cd entservices-apis && git checkout main && git checkout "$SRCREV_entservicesapis" || { echo "❌ Git clone failed."; exit 1; }
else
    echo "SRCREV_entservicesapis not found in $BB_FILE"
    exit 1
fi

# Path to the file containing SRC_URI entries
input_file="$BB_FILE"

# Base directory where patches are located
patch_base_dir="$SCRIPTS_DIR/meta-rdk-video/recipes-extended/wpe-framework/entservices-apis"

# Extract all .patch entries from multi-line SRC_URI blocks
patches=$(awk '
    BEGIN { in_src_uri = 0 }
    /SRC_URI/ { in_src_uri = 1 }
    in_src_uri {
        for (i = 1; i <= NF; i++) {
            if ($i ~ /file:\/\/.*\.patch/) {
                match($i, /file:\/\/[^ ]*\.patch/)
                print substr($i, RSTART, RLENGTH)
            }
        }
        if ($0 !~ /\\$/) {
            in_src_uri = 0
        }
    }
' "$input_file")

cd "$SCRIPTS_DIR/entservices-apis"
echo "$patches" | while read -r patch_uri; do
    patch_file="${patch_uri#file://}"
    full_path="$patch_base_dir/$patch_file"

    if [ -f "$full_path" ]; then
        echo "Applying patch: $full_path"
        git apply "$full_path"
        echo "✅ Successfully applied: $full_path"
    else
        echo "⚠️ Failed to apply: $full_path — skipping"
    fi
done

echo -e "${YELLOW}========================================Build entservices-apis===============================================${NC}"

cd "$SCRIPTS_DIR"

cmake -G Ninja -S entservices-apis -B entservices-apis/build \
    -DCMAKE_INSTALL_PREFIX="${WORKSPACE}/install/usr" \
    -DCMAKE_BUILD_TYPE="Debug" \
    -DTOOLS_SYSROOT="${PWD}"

# sudo cmake --build entservices-apis/build --target install
sudo ninja -C entservices-apis/build install

echo -e "${YELLOW}========================================Build ThunderClientLibrary===============================================${NC}"
# rm $SCRIPTS_DIR/install/include/WPEFramework/com/Ids.h #Due to Previous declaration error

#fatal error: interfaces/IPowerManager.h
sudo mkdir -p /usr/include/interfaces
sudo cp $SCRIPTS_DIR/entservices-apis/apis/PowerManager/IPowerManager.h /usr/include/interfaces
sudo cp $SCRIPTS_DIR/entservices-apis/apis/Ids.h /usr/include/interfaces
sudo cp $SCRIPTS_DIR/entservices-apis/apis/Module.h /usr/include/interfaces
sudo cp $SCRIPTS_DIR/entservices-apis/apis/entservices_errorcodes.h /usr/include/interfaces

cd $SCRIPTS_DIR
sudo cmake -G Ninja -S ThunderClientLibraries -B build/ThunderClientLibraries \
        -DCMAKE_INSTALL_PREFIX="${WORKSPACE}/install/usr" \
        -DCMAKE_PREFIX_PATH="${WORKSPACE}/install/usr" \
        -DCMAKE_MODULE_PATH="${WORKSPACE}/install/usr/include/WPEFramework/Modules" \
        -DTOOLS_SYSROOT="${PWD}" \
        -DBLUETOOTHAUDIOSINK=OFF \
        -DDEVICEINFO=OFF \
        -DDISPLAYINFO=OFF \
        -DSECURITYAGENT=OFF \
        -DPLAYERINFO=OFF \
        -DPROTOCOLS=OFF \
        -DVIRTUALINPUT=OFF \
        -DCOMPOSITORCLIENT=OFF \
        -DPOWERCONTROLLER=ON 

sudo cmake --build build/ThunderClientLibraries --target install

echo -e "${GREEN}========================================Clone Deps===============================================${NC}"
cd $SCRIPTS_DIR && ./build_deps.sh 

cd $WORKSPACE
echo -e "${GREEN}========================================Building flux===============================================${NC}"
(cd $WORKSPACE/deps/third-party/flux && autoreconf; autoreconf -f -i && ./configure && make && sudo make install && cd -)

echo -e "${GREEN}========================================Building directfb===============================================${NC}"
(cd $WORKSPACE/deps/third-party/DirectFB && autoreconf; autoreconf -f -i && ./configure && make && sudo make install && cd -)

echo -e "${GREEN}========================================Building log4c===============================================${NC}"
mv $WORKSPACE/deps/third-party/log4c-1.2.4 $WORKSPACE/deps/third-party/log4c
(cd $WORKSPACE/deps/third-party/log4c && autoreconf; autoreconf -f -i && ./configure && make && sudo make install && cd -)

echo -e "${GREEN}========================================Building safeclib===============================================${NC}"
(cd $WORKSPACE/deps/third-party/safeclib && autoreconf; autoreconf -f -i && ./configure && make && sudo make install && cd -)

echo -e "${GREEN}========================================Building dbus===============================================${NC}"
mv $WORKSPACE/deps/third-party/dbus-1.6.8 $WORKSPACE/deps/third-party/dbus
(cd $WORKSPACE/deps/third-party/dbus && autoreconf; autoreconf -f -i && ./configure && make && sudo make install && cd -)

echo -e "${GREEN}========================================Build glib===============================================${NC}"
mv $WORKSPACE/deps/third-party/glib-2.72.3 $WORKSPACE/deps/third-party/glib
(cd $WORKSPACE/deps/third-party/glib && meson _build && ninja -C _build )

echo -e "${GREEN}========================================Clone meta-middleware-generic-support===============================================${NC}"
cd $SCRIPTS_DIR
git clone git@github.com:rdkcentral/meta-middleware-generic-support.git

echo -e "${GREEN}========================================Create PATH===============================================${NC}"
cd $WORKSPACE/deps/
mkdir rdk
cd rdk

echo -e "${GREEN}========================================Create flask===============================================${NC}"
git clone git@github.com:rdk-e/FLASK-for-Hal-Mock.git flask && git checkout rdk_service_flash && git checkout "01844e3a54445245d953ff7ba3094ea0aa250aaa"

echo -e "${GREEN}========================================Clone meta-rdk-oss-reference===============================================${NC}"
cd $SCRIPTS_DIR
git clone git@github.com:rdkcentral/meta-rdk-oss-reference.git

echo -e "${GREEN}========================================Move Files===============================================${NC}"
sudo cp $SCRIPTS_DIR/meta-rdk-oss-reference/recipes-common/safec-common-wrapper/files/safec_lib.h /usr/include
sudo cp $WORKSPACE/deps/third-party/safeclib/include/*.h /usr/include
sudo cp $WORKSPACE/deps/third-party/glib/glib/glib.h /usr/include
sudo cp $WORKSPACE/deps/third-party/glib/_build/glib/glibconfig.h /usr/include

sudo cp $WORKSPACE/deps/third-party/dbus/dbus/dbus-arch-deps.h /usr/include/dbus-1.0/dbus/

cd $WORKSPACE/deps/rdk
echo -e "${GREEN}========================================Clone iarmbus===============================================${NC}"
sudo rm -rf iarmbus

INC_FILE="$SCRIPTS_DIR/meta-middleware-generic-support/conf/include/generic-srcrev.inc"

SRCREV_iarmbus=$(grep "^SRCREV_iarmbus" "$INC_FILE" | cut -d'"' -f2)

cd $WORKSPACE/deps/rdk/
if [ -n "$SRCREV_iarmbus" ]; then
    echo "Found SRCREV: $SRCREV_iarmbus"
    # You can now use $SRCREV_iarmbus in git checkout or other commands
    git clone git@github.com:rdkcentral/iarmbus.git && cd iarmbus && git checkout "$SRCREV_iarmbus" || { echo "❌ Git clone failed."; exit 1; }
else
    echo "SRCREV_entservicesapis not found in $INC_FILE"
    exit 1
fi

cp $SCRIPTS_DIR/patches/rdkservices/iarmbus/build.sh $WORKSPACE/deps/rdk/iarmbus

echo -e "${GREEN}========================================Build IARM===============================================${NC}"
cp $SCRIPTS_DIR/patches/rdkservices/iarmbus/build.sh $WORKSPACE/deps/rdk/iarmbus/
(cd $WORKSPACE/deps/rdk/iarmbus/ && sudo ./build.sh)

echo -e "${GREEN}========================================Clone DeviceSettings===============================================${NC}"
INC_FILE="$SCRIPTS_DIR/meta-middleware-generic-support/conf/include/generic-srcrev.inc"

SRCREV_devicesettings=$(grep "^SRCREV_devicesettings" "$INC_FILE" | cut -d'"' -f2)

cd $WORKSPACE/deps/rdk/
rm -rf devicesettings
if [ -n "$SRCREV_devicesettings" ]; then
    echo "Found SRCREV: $SRCREV_devicesettings"
    # You can now use $SRCREV_iarmbus in git checkout or other commands
    git clone git@github.com:rdkcentral/devicesettings.git && cd devicesettings && git checkout "$SRCREV_devicesettings" || { echo "❌ Git clone failed."; exit 1; }
else
    echo "SRCREV_entservicesapis not found in $INC_FILE"
    exit 1
fi

echo -e "${GREEN}========================================Clone rdk-halif-device_settings===============================================${NC}"
cd $WORKSPACE/deps/rdk/
git clone git@github.com:rdkcentral/rdk-halif-device_settings.git
cd $WORKSPACE/deps/rdk/rdk-halif-device_settings
git apply $SCRIPTS_DIR/patches/rdkservices/halif/intErr.patch
sudo cp $WORKSPACE/deps/rdk/rdk-halif-device_settings/include/*.h $INCLUDE_DIR

echo -e "${GREEN}========================================Clone DeviceSettings===============================================${NC}"
cd $WORKSPACE/deps/rdk/devicesettings
rm -rf hal
mkdir -p hal 
cd hal
echo "CLONING ISSUE FIX"
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519_rdkcentral
ssh-add ~/.ssh/id_ed25519_rdke
ssh-add -l

git clone git@github.com:rdk-e/devicesettings-emulator.git
mv devicesettings-emulator src

cd $WORKSPACE/deps/rdk/devicesettings/hal/src
git apply $SCRIPTS_DIR/patches/rdkservices/devicesettings/dsVideoHal.patch
( cd $WORKSPACE/deps/rdk/devicesettings/hal/src && sudo make )

echo -e "${GREEN}========================================Build rfc===============================================${NC}"
INC_FILE="$SCRIPTS_DIR/meta-middleware-generic-support/conf/include/generic-srcrev.inc"

SRCREV_rfc=$(grep -oP '^SRCREV:pn-rfc\s*=\s*"\K[0-9a-f]+' "$INC_FILE")

cd $WORKSPACE/deps/rdk/
rm -rf rfc
if [ -n "$SRCREV_rfc" ]; then
    echo "Found SRCREV: $SRCREV_rfc"
    # You can now use $SRCREV_iarmbus in git checkout or other commands
    git clone git@github.com:rdkcentral/rfc.git && cd rfc && git checkout "$SRCREV_rfc" || { echo "❌ Git clone failed."; exit 1; }
else
    echo "SRCREV_entservicesapis not found in $INC_FILE"
    exit 1
fi

cd /usr/include/
sudo mkdir wdmp-c
sudo cp $WORKSPACE/deps/rdk/devicesettings/stubs/wdmp-c.h $INCLUDE_DIR/wdmp-c
sudo cp $WORKSPACE/deps/rdk/rfc/rfcMgr/gtest/mocks/*.h $INCLUDE_DIR

cd $WORKSPACE/deps/rdk/rfc
git apply $SCRIPTS_DIR/patches/rdkservices/rfc/rfc.patch
(cd $WORKSPACE/deps/rdk/rfc && autoreconf; autoreconf -f -i && ./configure && make && sudo make install )

echo -e "${GREEN}========================================Build DeviceSettings===============================================${NC}"

sudo cp $WORKSPACE/deps/rdk/iarmbus/core/include/*.h $INCLUDE_DIR
sudo cp $WORKSPACE/deps/rdk/devicesettings/stubs/*.h $INCLUDE_DIR
sudo cp $WORKSPACE/deps/rdk/rfc/rfcapi/rfcapi.h $INCLUDE_DIR

cd $WORKSPACE/deps/rdk/devicesettings/
git apply $SCRIPTS_DIR/patches/rdkservices/devicesettings/NewDS_Err.patch
# git apply $SCRIPTS_DIR/patches/rdkservices/devicesettings/LoggerInitError.patch

cp $SCRIPTS_DIR/patches/rdkservices/devicesettings/build.sh $WORKSPACE/deps/rdk/devicesettings/
chmod -Rf 777 $WORKSPACE/deps/rdk/devicesettings/build.sh
(cd $WORKSPACE/deps/rdk/devicesettings/ && ./build.sh)

echo -e "${GREEN}========================================Clone rdk-halif-power_manager===============================================${NC}"
cd $WORKSPACE/deps/rdk/
git clone git@github.com:rdkcentral/rdk-halif-power_manager.git

sudo cp $WORKSPACE/deps/rdk/rdk-halif-power_manager/include/*.h $INCLUDE_DIR

echo -e "${GREEN}========================================Clone iarmmgr===============================================${NC}"
INC_FILE="$SCRIPTS_DIR/meta-middleware-generic-support/conf/include/generic-srcrev.inc"

# SRCREV_iarmmgrs=$(grep '^SRCREV:pn-iarmmgrs' "$INC_FILE" | cut -d'"' -f2)
SRCREV_iarmmgrs=$(grep -oP '^SRCREV:pn-iarmmgrs\s*=\s*"\K[0-9a-f]+' "$INC_FILE")

cd $WORKSPACE/deps/rdk/
rm -rf iarmmgrs
if [ -n "$SRCREV_iarmmgrs" ]; then
    echo "Found SRCREV: $SRCREV_iarmmgrs"
    # You can now use $SRCREV_iarmbus in git checkout or other commands
    git clone git@github.com:rdkcentral/iarmmgrs.git && cd iarmmgrs && git checkout "$SRCREV_iarmmgrs" || { echo "❌ Git clone failed."; exit 1; }
else
    echo "SRCREV_entservicesapis not found in $INC_FILE"
    exit 1
fi

echo -e "${GREEN}========================================Build iarmmgr===============================================${NC}"
sudo cp $WORKSPACE/deps/rdk/iarmmgrs/mfr/include/*.h $INCLUDE_DIR
cp $WORKSPACE/deps/rdk/devicesettings/hal/src/libds-hal.so $WORKSPACE/deps/rdk/devicesettings/install/lib
# Build iarmmgr
cd $WORKSPACE/deps/rdk/iarmmgrs
git apply $SCRIPTS_DIR/patches/rdkservices/iarmmgrs/Newiarm.patch
cp $SCRIPTS_DIR/patches/rdkservices/iarmmgrs/build.sh $WORKSPACE/deps/rdk/iarmmgrs
(cd $WORKSPACE/deps/rdk/iarmmgrs && ./build.sh)

echo -e "${GREEN}========================================Build power-manager===============================================${NC}"
cd $WORKSPACE/deps/rdk/
git clone git@github.com:rdkcentral/power-manager.git

echo -e "${GREEN}========================================Build act-hdmicec===============================================${NC}"
cd $WORKSPACE/deps/rdk/
rm -rf act-hdmicec
git clone git@github.com:rdk-e/act-hdmicec.git
mv act-hdmicec hdmicec

cd $WORKSPACE/deps/rdk/hdmicec/soc/L2HalMock/
git clone git@github.com:rdk-e/hdmicec-hal-emulator.git && cd hdmicec-hal-emulator && git checkout "2e459b5cf75eed032b9957d5c8ffab1da6d6cc3b"

mv $WORKSPACE/deps/rdk/hdmicec/soc/L2HalMock/common $WORKSPACE/deps/rdk/hdmicec/soc/L2HalMock/common_bkp; 
mv $WORKSPACE/deps/rdk/hdmicec/soc/L2HalMock/hdmicec-hal-emulator $WORKSPACE/deps/rdk/hdmicec/soc/L2HalMock/common
sudo cp $WORKSPACE/deps/rdk/power-manager/source/include/pwrMgr.h $INCLUDE_DIR
sudo cp $WORKSPACE/deps/rdk/hdmicec/ccec/include/ccec/driver/hdmi_cec_driver.h $INCLUDE_DIR
sudo cp $WORKSPACE/deps/rdk/hdmicec/soc/L2HalMock/common/*.hpp $INCLUDE_DIR

cd $INCLUDE_DIR
sudo mkdir json
sudo cp $INCLUDE_DIR/jsoncpp/json/*.h $INCLUDE_DIR/json/

cp $SCRIPTS_DIR/patches/rdkservices/hdmicec/build.sh $WORKSPACE/deps/rdk/hdmicec
(cd $WORKSPACE/deps/rdk/hdmicec && ./build.sh)

echo -e "${GREEN}========================================Build HdmiCecSource===============================================${NC}"

if grep -q "HdmiCecSource" <<< "$SelectedPlugins"; then
sudo cp $SCRIPTS_DIR/patches/rdkservices/properties/HdmiCecSource/device.properties /etc/
fi

cd $WORKSPACE/
#Run time dependency 
mkdir -p $WORKSPACE/install/etc/WPEFramework/plugins
cp $SCRIPTS_DIR/patches/rdkservices/files/HdmiCecSource.json $WORKSPACE/install/etc/WPEFramework/plugins/
cp $SCRIPTS_DIR/patches/rdkservices/files/HdmiCecSink.json $WORKSPACE/install/etc/WPEFramework/plugins/

#included CmakeHelperFunctions.cmake instead to during in CMakeLists.txt
sudo cp ${WORKSPACE}/install/usr/lib/cmake/WPEFramework/common/CmakeHelperFunctions.cmake $WORKSPACE/install/usr/include/WPEFramework/Modules
cp -r $RDK_DIR/Tests/L2HALMockTests/. $WORKSPACE/deps/rdk/flask
cp $RDK_DIR/L2HalMock/patches/rdkservices/files/rfcapi.h $RDK_DIR/helpers

# cp $RDK_DIR/L2HalMock/patches/rdkservices/files/rfcapi.h $RDK_DIR/helpers
cd $INCLUDE_DIR
mkdir interfaces
sudo cp $SCRIPTS_DIR/entservices-apis/apis/HdmiCecSource/IHdmiCecSource.h $INCLUDE_DIR/interfaces

cd $RDK_DIR;
cmake -S . -B build \
-DCMAKE_INSTALL_PREFIX="$WORKSPACE/install/usr" \
-DCMAKE_MODULE_PATH="${WORKSPACE}/install/usr/include/WPEFramework/Modules" \
-DWPEFrameworkDefinitions_DIR="$SCRIPTS_DIR/install/lib/cmake/WPEFrameworkDefinitions" \
-DCompileSettingsDebug_DIR="$SCRIPTS_DIR/install/lib/cmake/CompileSettingsDebug" \
-DCEC_INCLUDE_DIRS="$SCRIPTS_DIR/workspace/deps/rdk/hdmicec/ccec/include" \
-DRDK_SERVICE_L2HALMOCK=ON \
-DUSE_THUNDER_R4=ON \
-DPLUGIN_HDMICECSOURCE=ON \
-DPLUGIN_HDMICECSINK=OFF \
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
