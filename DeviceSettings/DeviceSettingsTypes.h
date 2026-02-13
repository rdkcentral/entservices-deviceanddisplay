/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#pragma once

#include <functional>
#include <map>
#include <iostream>
#include <fstream>
#include <exception>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <interfaces/IDeviceSettings.h>
#include <interfaces/IDeviceSettingsAudio.h>
#include <interfaces/IDeviceSettingsCompositeIn.h>
#include <interfaces/IDeviceSettingsDisplay.h>
#include <interfaces/IDeviceSettingsFPD.h>
#include <interfaces/IDeviceSettingsHDMIIn.h>
#include <interfaces/IDeviceSettingsHost.h>
#include <interfaces/IDeviceSettingsVideoDevice.h>
#include <interfaces/IDeviceSettingsVideoPort.h>

#define USE_LEGACY_INTERFACE

#ifdef USE_LEGACY_INTERFACE
using DeviceSetting               = WPEFramework::Exchange::IDeviceSettings;
using DeviceSettingsFPD            = WPEFramework::Exchange::IDeviceSettingsFPD;
using DeviceSettingsHDMIIn         = WPEFramework::Exchange::IDeviceSettingsHDMIIn;
using DeviceSettingsCompositeIn    = WPEFramework::Exchange::IDeviceSettingsCompositeIn;
using DeviceSettingsAudio          = WPEFramework::Exchange::IDeviceSettingsAudio;
using DeviceSettingsVideoDevice    = WPEFramework::Exchange::IDeviceSettingsVideoDevice;
using DeviceSettingsDisplay        = WPEFramework::Exchange::IDeviceSettingsDisplay;
using DeviceSettingsHost           = WPEFramework::Exchange::IDeviceSettingsHost;
using DeviceSettingsVideoPort      = WPEFramework::Exchange::IDeviceSettingsVideoPort;
#else
using DeviceSettingsManagerFPD            = WPEFramework::Exchange::IDeviceSettingsManager::IFPD;
using DeviceSettingsManagerHDMIIn         = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn;
using DeviceSettingsManagerCompositeIn    = WPEFramework::Exchange::IDeviceSettingsManager::ICompositeIn;
using DeviceSettingsManagerAudio          = WPEFramework::Exchange::IDeviceSettingsManager::IAudio;
using DeviceSettingsManagerVideoDevice    = WPEFramework::Exchange::IDeviceSettingsManager::IVideoDevice;
using DeviceSettingsManagerDisplay        = WPEFramework::Exchange::IDeviceSettingsManager::IDisplay;
using DeviceSettingsManagerHost           = WPEFramework::Exchange::IDeviceSettingsManager::IHost;
using DeviceSettingsManagerVideoPort      = WPEFramework::Exchange::IDeviceSettingsManager::IVideoPort;
#endif

// HDMI In type aliases for convenience
using HDMIInPort               = DeviceSettingsHDMIIn::HDMIInPort;
using HDMIInSignalStatus       = DeviceSettingsHDMIIn::HDMIInSignalStatus;
using HDMIVideoPortResolution  = DeviceSettingsHDMIIn::HDMIVideoPortResolution;
using HDMIInAviContentType     = DeviceSettingsHDMIIn::HDMIInAviContentType;
using HDMIInVRRType            = DeviceSettingsHDMIIn::HDMIInVRRType;
using HDMIInStatus             = DeviceSettingsHDMIIn::HDMIInStatus;
using HDMIVideoPlaneType       = DeviceSettingsHDMIIn::HDMIVideoPlaneType;
using HDMIInVRRStatus          = DeviceSettingsHDMIIn::HDMIInVRRStatus;
using HDMIInCapabilityVersion  = DeviceSettingsHDMIIn::HDMIInCapabilityVersion;
using HDMIInEdidVersion        = DeviceSettingsHDMIIn::HDMIInEdidVersion;
using HDMIInVideoZoom          = DeviceSettingsHDMIIn::HDMIInVideoZoom;
using HDMIInVideoRectangle     = DeviceSettingsHDMIIn::HDMIInVideoRectangle;
using HDMIVideoAspectRatio     = DeviceSettingsHDMIIn::HDMIVideoAspectRatio;
using HDMIInTVResolution       = DeviceSettingsHDMIIn::HDMIInTVResolution;
using HDMIInVideoStereoScopicMode = DeviceSettingsHDMIIn::HDMIInVideoStereoScopicMode;
using HDMIInVideoFrameRate     = DeviceSettingsHDMIIn::HDMIInVideoFrameRate;
using IHDMIInPortConnectionStatusIterator = DeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator;using IHDMIInGameFeatureListIterator      = DeviceSettingsHDMIIn::IHDMIInGameFeatureListIterator;
using GameFeatureListIteratorImpl = WPEFramework::Core::Service<WPEFramework::RPC::IteratorType<IHDMIInGameFeatureListIterator>>;

// FPD type aliases for convenience
using FPDTimeFormat = DeviceSettingsFPD::FPDTimeFormat;
using FPDIndicator = DeviceSettingsFPD::FPDIndicator;
using FPDState = DeviceSettingsFPD::FPDState;
using FPDTextDisplay = DeviceSettingsFPD::FPDTextDisplay;
using FPDMode = DeviceSettingsFPD::FPDMode;
using FDPLEDState = DeviceSettingsFPD::FDPLEDState;

// Common constants
#define API_VERSION_MAJOR 1
#define API_VERSION_MINOR 0
#define API_VERSION_PATCH 0

#define TVSETTINGS_DALS_RFC_PARAM "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TvSettings.DynamicAutoLatency"
#define RDK_DSHAL_NAME "libds-hal.so"

#ifdef DEBUG_LOGGING
#define ENTRY_LOG do { LOGINFO("%d: Enter %s", __LINE__, __func__); } while(0);
#define EXIT_LOG do { LOGINFO("%d: Exit %s", __LINE__, __func__); } while(0);
#else
#define ENTRY_LOG do { } while(0)
#define EXIT_LOG do { } while(0)
#endif

#ifdef DEBUG_LOGGING
#define DEBUG_LOG(fmt, ...) LOGINFO(fmt, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...) do { } while(0)
#endif

// Exact replica of original HostPersistence implementation to avoid DS_LIBRARIES dependency
namespace device {
    class HostPersistence {
    private:
        std::map<std::string, std::string> _properties;
        std::map<std::string, std::string> _defaultProperties;
        std::string filePath;
        std::string defaultFilePath;
        bool _isInitialized = false;

        void ensureInitialized() {
            if (!_isInitialized) {
                load();
                _isInitialized = true;
            }
        }

        void loadFromFile(const std::string &fileName, std::map<std::string, std::string> &map) {
            char keyValue[1024] = "";
            char key[1024] = "";
            FILE *filePtr = NULL;

            filePtr = fopen(fileName.c_str(), "r");
            if (filePtr != NULL) {
                while (!feof(filePtr)) {
                    /* RDKSEC-811 Coverity fix - CHECKED_RETURN */
                    if (fscanf(filePtr, "%1023s\t%1023s\n", key, keyValue) <= 0) {
                        // fscanf failed
                    } else {
                        /* Check the TypeOfInput variable and then call the appropriate insert function */
                        map.insert({key, keyValue});
                    }
                }
                fclose(filePtr);
            } else {
                // File doesn't exist - this is okay for initial startup
            }
        }

        void writeToFile(const std::string &fileName) {
            unlink(fileName.c_str());

            if (_properties.size() > 0) {
                /*
                 * Replacing the ofstream to fwrite
                 * Because the ofstream.close or ofstream.flush or ofstream.rdbuf->sync
                 * does not sync the data onto disk.
                 * TBD - This need to be changed to C++ APIs in future.
                 */

                FILE *file = fopen(fileName.c_str(), "w");
                if (file != NULL) {
                    for (auto it = _properties.begin(); it != _properties.end(); ++it) {
                        std::string dataToWrite = it->first + "\t" + it->second + "\n";
                        unsigned int size = dataToWrite.length();
                        fwrite(dataToWrite.c_str(), 1, size, file);
                    }

                    fflush(file);           // Flush buffers to FS
                    fsync(fileno(file));    // Flush file to HDD
                    fclose(file);
                }
            }
        }

    public:
        HostPersistence() {
            /*
             * TBD This need to be removed and 
             * Persistent path shall be set from startup script
             * To do this Host Persistent need to be part of DS Manager
             * TBD
             */

            #if defined(HAS_HDD_PERSISTENT)
                /*Product having HDD Persistent*/
                filePath = "/tmp/mnt/diska3/persistent/ds/hostData";
            #elif defined(HAS_FLASH_PERSISTENT)
                /*Product having Flash Persistent*/
                filePath = "/opt/persistent/ds/hostData";
            #else
                filePath = "/opt/ds/hostData";
                /*Default case*/
            #endif
            defaultFilePath = "/etc/hostDataDefault";
            _isInitialized = true;
        }

        HostPersistence(const std::string &storeFileName) {
            filePath = storeFileName;
            defaultFilePath = "/etc/hostDataDefault";
            _isInitialized = true;
        }

        virtual ~HostPersistence() {
            // Auto-generated destructor stub
        }

        static HostPersistence& getInstance() {
            static HostPersistence instance;
            return instance;
        }

        void load() {
            try {
                loadFromFile(filePath, _properties);
            } catch (...) {
                // Backup file is corrupt or not available
                try {
                    loadFromFile(filePath + "tmpDB", _properties);
                } catch (...) {
                    /* Remove all properties, and start with default values */
                }
            }

            try {
                loadFromFile(defaultFilePath, _defaultProperties);
            } catch (...) {
                // System file is corrupt or not available
            }
        }

        std::string getProperty(const std::string &key) {
            /* Ensure data is loaded before accessing properties */
            ensureInitialized();

            /* Check the validness of the key */
            if (key.empty()) {
                throw std::invalid_argument("The KEY is empty");
            }

            std::map<std::string, std::string>::const_iterator eFound = _properties.find(key);
            if (eFound == _properties.end()) {
                throw std::invalid_argument("The Item IS NOT FOUND");
            } else {
                return eFound->second;
            }
        }

        std::string getProperty(const std::string &key, const std::string &defValue) {
            /* Ensure data is loaded before accessing properties */
            ensureInitialized();

            /* Check the validness of the key */
            if (key.empty()) {
                throw std::invalid_argument("The KEY is empty");
            }

            std::map<std::string, std::string>::const_iterator eFound = _properties.find(key);
            if (eFound == _properties.end()) {
                return defValue;
            } else {
                return eFound->second;
            }
        }

        std::string getDefaultProperty(const std::string &key) {
            /* Ensure data is loaded before accessing properties */
            ensureInitialized();

            /* Check the validness of the key */
            if (key.empty()) {
                throw std::invalid_argument("The KEY is empty");
            }

            std::map<std::string, std::string>::const_iterator eFound = _defaultProperties.find(key);
            if (eFound == _defaultProperties.end()) {
                throw std::invalid_argument("The Item IS NOT FOUND");
            } else {
                return eFound->second;
            }
        }

        void persistHostProperty(const std::string &key, const std::string &value) {
            /* Ensure data is loaded before accessing properties */
            ensureInitialized();

            if (key.empty() || value.empty()) {
                throw std::invalid_argument("Given KEY or VALUE is empty");
            }

            try {
                std::string eRet = getProperty(key);

                if (eRet.compare(value) == 0) {
                    /* Same value. No need to do anything */
                    return;
                }

                /* Save a current copy before modifying */
                writeToFile(filePath + "tmpDB");

                /* First of all check whether the entry is already present in the hashtable */
                _properties.erase(key);

            } catch (const std::invalid_argument &e) {
                // Entry Not found
            } catch (...) {
                // Other exceptions
            }

            _properties.insert({key, value});
            writeToFile(filePath);
        }
    };
}

struct CallbackBundle {
    std::function<void(HDMIInPort, bool)> OnHDMIInHotPlugEvent;
    std::function<void(HDMIInPort, HDMIInSignalStatus)> OnHDMIInSignalStatusEvent;
    std::function<void(HDMIInPort, bool)> OnHDMIInStatusEvent;
    std::function<void(HDMIInPort, HDMIVideoPortResolution)> OnHDMIInVideoModeUpdateEvent;
    std::function<void(HDMIInPort, bool)> OnHDMIInAllmStatusEvent;
    std::function<void(HDMIInPort, HDMIInAviContentType)> OnHDMIInAVIContentTypeEvent;
    std::function<void(int32_t, int32_t)> OnHDMIInAVLatencyEvent;
    std::function<void(HDMIInPort, HDMIInVRRType)> OnHDMIInVRRStatusEvent;
    // Add other callbacks as needed
};
