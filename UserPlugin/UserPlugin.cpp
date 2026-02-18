/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 */

#include "UserPlugin.h"
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include <chrono>
#include "UtilsLogging.h"
#include "audioOutputPort.hpp"
#include "audioOutputPortType.hpp"
#include "audioOutputPortConfig.hpp"
#include "dsUtl.h"
#include "dsError.h"
#include "list.hpp"
#include "host.hpp"
#include "exception.hpp"
#include "manager.hpp"

// IARM includes for direct DsMgr daemon communication
#include "libIBus.h"
#include "rdk/ds-rpc/dsRpc.h"
#include "rdk/ds-rpc/dsMgr.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 1
#define API_VERSION_NUMBER_PATCH 0

using namespace std;
using namespace WPEFramework;

namespace WPEFramework {
namespace {
    static Plugin::Metadata<Plugin::UserPlugin> metadata(
        // Version (Major, Minor, Patch)
        API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
        // Preconditions
        {},
        // Terminations
        {},
        // Controls
        {}
    );
}

namespace Plugin {

    SERVICE_REGISTRATION(UserPlugin, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    UserPlugin* UserPlugin::_instance = nullptr;

    UserPlugin::UserPlugin() : _service(nullptr), _connectionId(0), _fpdManager(nullptr), _hdmiInManager(nullptr), _hdmiInNotification(*this)
    {
        UserPlugin::_instance = this;
        SYSLOG(Logging::Startup, (_T("UserPlugin Constructor")));
        LOGINFO("Constructor");
    }

    UserPlugin::~UserPlugin()
    {
        UserPlugin::_instance = nullptr;
        SYSLOG(Logging::Shutdown, (string(_T("UserPlugin Destructor"))));
        LOGINFO("Destructor");
    }

    /* virtual */ const string UserPlugin::Initialize(PluginHost::IShell* service)
    {
        LOGINFO("Initialize");
        _service = service;
        _service->AddRef();

        // Get DeviceSettings interface
        _deviceSettings = _service->QueryInterfaceByCallsign<Exchange::IDeviceSettings>("org.rdk.DeviceSettings");
        if (_deviceSettings != nullptr) {
            // Get individual interfaces from the main DeviceSettings interface
            _fpdManager = _service->QueryInterfaceByCallsign<Exchange::IDeviceSettingsFPD>("org.rdk.DeviceSettings");
            _hdmiInManager = _service->QueryInterfaceByCallsign<Exchange::IDeviceSettingsHDMIIn>("org.rdk.DeviceSettings");
        } else {
            LOGERR("Could not obtain DeviceSettings interface");
        }

        // Register for HDMI In notifications if interface is available
        if (_hdmiInManager) {
            _hdmiInManager->Register(&_hdmiInNotification);
        }

        // Test DeviceSettings interfaces
        TestFPDAPIs();
        TestSpecificHDMIInAPIs();

        // Test IARM APIs for direct DsMgr daemon communication
        //TestIARMHdmiInAPIs();

        // Test additional IARM APIs (AV Latency, ALLM, EDID, VRR)
        //TestIARMAdditionalAPIs();

        Exchange::JUserPlugin::Register(*this, this);

        return (string());
    }

    /* virtual */ void UserPlugin::Deinitialize(PluginHost::IShell* service)
    {
        LOGINFO("Deinitialize");

        // Unregister HDMI In notifications and release HDMI In Manager
        if (_hdmiInManager) {
            _hdmiInManager->Unregister(&_hdmiInNotification);
            _hdmiInManager->Release();
            _hdmiInManager = nullptr;
        }

        // Release FPD Manager
        if (_fpdManager) {
            _fpdManager->Release();
            _fpdManager = nullptr;
        }

        // Release main DeviceSettings interface
        if (_deviceSettings) {
            _deviceSettings->Release();
            _deviceSettings = nullptr;
        }

        Exchange::JUserPlugin::Unregister(*this);
        _service = nullptr;
    }

    /***
     * @brief : send notification when system power state is changed
     *
     * @param1[in]  : powerState
     * @param2[out] : {"jsonrpc": "2.0",
     *		"method": "org.rdk.SystemServices.events.1.onSystemPowerStateChanged",
     *		"param":{"powerState": <string new power state mode>}}
    */
    void UserPlugin::onPowerStateChanged(string currentPowerState, string powerState)
    {
        LOGWARN("power state changed from '%s' to '%s'", currentPowerState.c_str(), powerState.c_str()); 
    }

    /* virtual */ string UserPlugin::Information() const
    {
        // No additional info to report.
        return (string());
    }


    // PowerManager functionality moved to separate plugin - simplified GetDevicePowerState implementation
    Core::hresult UserPlugin::GetDevicePowerState(std::string& powerState) const
    {
        // PowerManager functionality moved to separate plugin, return default state
        powerState = "UNKNOWN";
        LOGINFO("PowerManager functionality moved to separate plugin - returning UNKNOWN state");
        return Core::ERROR_NONE;
    }

    // PowerManager functionality moved to separate plugin - method commented out
    /*
    void UserPlugin::OnPowerModeChanged(const WPEFramework::Exchange::IPowerManager::PowerState currentState, const WPEFramework::Exchange::IPowerManager::PowerState newState)
    {
        std::string curPowerState,newPowerState = "";

        curPowerState = std::to_string(currentState);
        newPowerState = std::to_string(newState);

        LOGWARN("OnPowerModeChanged, Old State %s, New State: %s\n", curPowerState.c_str(), newPowerState.c_str());
        if (UserPlugin::_instance) {
            UserPlugin::_instance->onPowerStateChanged(curPowerState, newPowerState);
        } else {
            LOGERR("UserPlugin::_instance is NULL.\n");
        }
    }
    */

    Core::hresult UserPlugin::GetVolumeLevel (const string& port, string& level) const
    {
        //LOGINFOMETHOD();
        //string audioPort = port.String();
        //device::List<device::AudioOutputPort> aPorts = device::Host::getInstance().getAudioOutputPorts();
        float volumelevel = 0;

        LOGINFO("GetVolumeLevel, port name: %s", port.c_str());
        device::AudioOutputPort aPort = device::Host::getInstance().getAudioOutputPort(port);
        if (aPort.getName().empty()){
            LOGERR("AudioOutputPort '%s' is not valid!", port.c_str());
            return Core::ERROR_GENERAL;
        }
            volumelevel = aPort.getLevel();
            level = to_string(volumelevel);
            LOGINFO("GetVolumeLevel, volumeLevel: %s", level.c_str());
    //    volumelevel = aPort.getLevel();

        return Core::ERROR_NONE;
    }

    void UserPlugin::TestFPDAPIs()
    {
        LOGINFO("========== FPD APIs Testing Framework ==========");

        if (!_fpdManager) {
            LOGERR("FPD Manager interface is not available");
            return;
        }

        LOGINFO("Testing DeviceSettings FPD APIs...");

        // Test all FPD indicators
        Exchange::IDeviceSettingsFPD::FPDIndicator testIndicators[] = {
            Exchange::IDeviceSettingsFPD::FPDIndicator::DS_FPD_INDICATOR_MESSAGE,
            Exchange::IDeviceSettingsFPD::FPDIndicator::DS_FPD_INDICATOR_POWER,
            Exchange::IDeviceSettingsFPD::FPDIndicator::DS_FPD_INDICATOR_RECORD,
            Exchange::IDeviceSettingsFPD::FPDIndicator::DS_FPD_INDICATOR_REMOTE,
            Exchange::IDeviceSettingsFPD::FPDIndicator::DS_FPD_INDICATOR_RFBYPASS
        };

        const char* indicatorNames[] = {
            "MESSAGE", "POWER", "RECORD", "REMOTE", "RFBYPASS"
        };

        // Test FPD Brightness APIs
        for (size_t i = 0; i < sizeof(testIndicators)/sizeof(testIndicators[0]); i++) {
            auto indicator = testIndicators[i];
            const char* indicatorName = indicatorNames[i];

            LOGINFO("---------- Testing FPD Indicator: %s ----------", indicatorName);

            // Get current brightness
            uint32_t currentBrightness = 0;
            Core::hresult result = _fpdManager->GetFPDBrightness(indicator, currentBrightness);
            LOGINFO("GetFPDBrightness: indicator=%s, result=%u, brightness=%u", indicatorName, result, currentBrightness);

            if (result == Core::ERROR_NONE) {
                // Test Set brightness
                uint32_t testBrightness = (currentBrightness == 50) ? 75 : 50;
                result = _fpdManager->SetFPDBrightness(indicator, testBrightness, false);
                LOGINFO("SetFPDBrightness: indicator=%s, result=%u, brightness=%u", indicatorName, result, testBrightness);

                // Restore original brightness
                _fpdManager->SetFPDBrightness(indicator, currentBrightness, false);
            }
        }

        // Test FPD State APIs
        for (size_t i = 0; i < sizeof(testIndicators)/sizeof(testIndicators[0]); i++) {
            auto indicator = testIndicators[i];
            const char* indicatorName = indicatorNames[i];

            // Get current state
            Exchange::IDeviceSettingsFPD::FPDState currentState;
            Core::hresult result = _fpdManager->GetFPDState(indicator, currentState);
            LOGINFO("GetFPDState: indicator=%s, result=%u, state=%u", indicatorName, result, (uint32_t)currentState);

            if (result == Core::ERROR_NONE) {
                // Test Set state
                Exchange::IDeviceSettingsFPD::FPDState testState = Exchange::IDeviceSettingsFPD::FPDState::DS_FPD_STATE_ON;
                result = _fpdManager->SetFPDState(indicator, testState);
                LOGINFO("SetFPDState: indicator=%s, result=%u, state=ON", indicatorName, result);

                // Restore original state
                _fpdManager->SetFPDState(indicator, currentState);
            }
        }

        // Test FPD Color APIs
        for (size_t i = 0; i < sizeof(testIndicators)/sizeof(testIndicators[0]); i++) {
            auto indicator = testIndicators[i];
            const char* indicatorName = indicatorNames[i];

            // Get current color
            uint32_t currentColor = 0;
            Core::hresult result = _fpdManager->GetFPDColor(indicator, currentColor);
            LOGINFO("GetFPDColor: indicator=%s, result=%u, color=0x%X", indicatorName, result, currentColor);

            if (result == Core::ERROR_NONE) {
                // Test Set color
                uint32_t testColor = 0xFF0000; // Red
                result = _fpdManager->SetFPDColor(indicator, testColor);
                LOGINFO("SetFPDColor: indicator=%s, result=%u, color=0x%X", indicatorName, result, testColor);

                // Restore original color
                _fpdManager->SetFPDColor(indicator, currentColor);
            }
        }

        // Test FPD Time Format APIs
        Exchange::IDeviceSettingsFPD::FPDTimeFormat currentTimeFormat;
        Core::hresult result = _fpdManager->GetFPDTimeFormat(currentTimeFormat);
        LOGINFO("GetFPDTimeFormat: result=%u, format=%u", result, (uint32_t)currentTimeFormat);

        if (result == Core::ERROR_NONE) {
            // Test Set time format
            Exchange::IDeviceSettingsFPD::FPDTimeFormat testFormat = Exchange::IDeviceSettingsFPD::FPDTimeFormat::DS_FPD_TIMEFORMAT_24_HOUR;
            result = _fpdManager->SetFPDTimeFormat(testFormat);
            LOGINFO("SetFPDTimeFormat: result=%u, format=24_HOUR", result);

            // Restore original format
            _fpdManager->SetFPDTimeFormat(currentTimeFormat);
        }

        // Test FPD Text Brightness APIs
        Exchange::IDeviceSettingsFPD::FPDTextDisplay textDisplay = Exchange::IDeviceSettingsFPD::FPDTextDisplay::DS_FPD_TEXTDISPLAY_TEXT;
        const char* displayName = "TEXT";

        // Get current text brightness
        uint32_t currentTextBrightness = 0;
        result = _fpdManager->GetFPDTextBrightness(textDisplay, currentTextBrightness);
        LOGINFO("GetFPDTextBrightness: display=%s, result=%u, brightness=%u", displayName, result, currentTextBrightness);

        if (result == Core::ERROR_NONE) {
            // Test Set text brightness
            uint32_t testTextBrightness = (currentTextBrightness == 50) ? 75 : 50;
            result = _fpdManager->SetFPDTextBrightness(textDisplay, testTextBrightness);
            LOGINFO("SetFPDTextBrightness: display=%s, result=%u, brightness=%u", displayName, result, testTextBrightness);

            // Restore original text brightness
            _fpdManager->SetFPDTextBrightness(textDisplay, currentTextBrightness);
        }

        // Test FPD Clock Display
        result = _fpdManager->EnableFPDClockDisplay(true);
        LOGINFO("EnableFPDClockDisplay(true): result=%u", result);
        
        result = _fpdManager->EnableFPDClockDisplay(false);
        LOGINFO("EnableFPDClockDisplay(false): result=%u", result);

        // Test FPD Blink API
        result = _fpdManager->SetFPDBlink(Exchange::IDeviceSettingsFPD::FPDIndicator::DS_FPD_INDICATOR_POWER, 1000, 3);
        LOGINFO("SetFPDBlink: result=%u (duration=1000ms, iterations=3)", result);

        // Test FPD Scroll API
        result = _fpdManager->SetFPDScroll(2000, 2, 2);
        LOGINFO("SetFPDScroll: result=%u (hold=2000ms, h_scroll=2, v_scroll=2)", result);

        // Test FPD Time API
        result = _fpdManager->SetFPDTime(Exchange::IDeviceSettingsFPD::FPDTimeFormat::DS_FPD_TIMEFORMAT_12_HOUR, 30, 45);
        LOGINFO("SetFPDTime: result=%u (12_HOUR format, 30:45)", result);

        // Test FPD Mode API
        result = _fpdManager->SetFPDMode(Exchange::IDeviceSettingsFPD::FPDMode::DS_FPD_MODE_CLOCK);
        LOGINFO("SetFPDMode: result=%u (CLOCK mode)", result);

        LOGINFO("========== FPD APIs Testing Completed ==========\\n");
    }

    void UserPlugin::TestSpecificHDMIInAPIs()
    {
        LOGINFO("========== HDMI In APIs Testing Framework ==========");

        if (!_hdmiInManager) {
            LOGERR("HDMI In Manager interface is not available!");
            return;
        }

        LOGINFO("HDMI In Manager interface available (DeviceSettings: %s)", _deviceSettings ? "Available" : "Not Available");

        LOGINFO("Testing DeviceSettings HDMI In APIs via QueryInterface architecture...");

        // 1. Test GetHDMIInNumbefOfInputs
        int32_t inputCount = 0;
        Core::hresult result = _hdmiInManager->GetHDMIInNumbefOfInputs(inputCount);
        LOGINFO("GetHDMIInNumbefOfInputs: result=%u, count=%d", result, inputCount);

        // 2. Test GetHDMIInStatus
        DeviceSettingsHDMIIn::HDMIInStatus hdmiStatus;
        DeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator* portStatus = nullptr;
        result = _hdmiInManager->GetHDMIInStatus(hdmiStatus, portStatus);
        LOGINFO("GetHDMIInStatus: result=%u, activePort=%d, isPresented=%s", 
               result, hdmiStatus.activePort, hdmiStatus.isPresented ? "true" : "false");
        if (portStatus) {
            portStatus->Release();
        }

        // 3. Test GetHDMIInAVLatency
        uint32_t videoLatency = 0, audioLatency = 0;
        result = _hdmiInManager->GetHDMIInAVLatency(videoLatency, audioLatency);
        LOGINFO("GetHDMIInAVLatency: result=%u, videoLatency=%u, audioLatency=%u", 
               result, videoLatency, audioLatency);

        // 4. Test GetHDMIInAllmStatus for HDMI Port 0
        bool allmStatus = false;
        result = _hdmiInManager->GetHDMIInAllmStatus(DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_0, allmStatus);
        LOGINFO("GetHDMIInAllmStatus(Port0): result=%u, allmStatus=%s", 
               result, allmStatus ? "true" : "false");

        // 5. Test GetHDMIInEdid2AllmSupport for HDMI Port 0
        bool allmSupport = false;
        result = _hdmiInManager->GetHDMIInEdid2AllmSupport(DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_0, allmSupport);
        LOGINFO("GetHDMIInEdid2AllmSupport(Port0): result=%u, allmSupport=%s", 
               result, allmSupport ? "true" : "false");

        // 6. Test GetHDMIEdidVersion for HDMI Port 0
        DeviceSettingsHDMIIn::HDMIInEdidVersion edidVersion;
        result = _hdmiInManager->GetHDMIEdidVersion(DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_0, edidVersion);
        LOGINFO("GetHDMIEdidVersion(Port0): result=%u, edidVersion=%d", result, edidVersion);

        // 7. Test GetHDMIVideoMode
        DeviceSettingsHDMIIn::HDMIVideoPortResolution videoMode;
        result = _hdmiInManager->GetHDMIVideoMode(videoMode);
        LOGINFO("GetHDMIVideoMode: result=%u, name='%s', pixelRes=%d, aspectRatio=%d, frameRate=%d", 
               result, videoMode.name.c_str(), videoMode.pixelResolution, 
               videoMode.aspectRatio, videoMode.frameRate);

        // 8. Test GetHDMIVersion for HDMI Port 0
        DeviceSettingsHDMIIn::HDMIInCapabilityVersion hdmiVersion;
        result = _hdmiInManager->GetHDMIVersion(DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_0, hdmiVersion);
        LOGINFO("GetHDMIVersion(Port0): result=%u, version=%d", result, hdmiVersion);

        // 9. Test GetVRRSupport for HDMI Port 0
        bool vrrSupport = false;
        result = _hdmiInManager->GetVRRSupport(DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_0, vrrSupport);
        LOGINFO("GetVRRSupport(Port0): result=%u, vrrSupport=%s", 
               result, vrrSupport ? "true" : "false");

        // 10. Test GetVRRStatus for HDMI Port 0
        DeviceSettingsHDMIIn::HDMIInVRRStatus vrrStatus;
        result = _hdmiInManager->GetVRRStatus(DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_0, vrrStatus);
        LOGINFO("GetVRRStatus(Port0): result=%u, vrrType=%d, vrrFreeSyncFramerate=%.2f", 
               result, vrrStatus.vrrType, vrrStatus.vrrFreeSyncFramerateHz);

        // 11. Test GetSupportedGameFeaturesList
        DeviceSettingsHDMIIn::IHDMIInGameFeatureListIterator* gameFeatures = nullptr;
        result = _hdmiInManager->GetSupportedGameFeaturesList(gameFeatures);
        LOGINFO("GetSupportedGameFeaturesList: result=%u", result);
        if (gameFeatures) {
            LOGINFO("Game features list obtained successfully");
            // Iterate through features if needed
            gameFeatures->Release();
        }

        // 12. Test EDID bytes (first 256 bytes)
        uint8_t edidBytes[256];
        result = _hdmiInManager->GetEdidBytes(DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_0, 256, edidBytes);
        LOGINFO("GetEdidBytes(Port0, 256 bytes): result=%u", result);
        if (result == Core::ERROR_NONE) {
            LOGINFO("EDID bytes retrieved successfully for Port 0");
        }

        // 13. Test SPD Information (28 bytes for SPD InfoFrame)
        uint8_t spdBytes[28];
        result = _hdmiInManager->GetHDMISPDInformation(DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_0, 28, spdBytes);
        LOGINFO("GetHDMISPDInformation(Port0, 28 bytes): result=%u", result);
        if (result == Core::ERROR_NONE) {
            LOGINFO("SPD information retrieved successfully for Port 0");
        }

        LOGINFO("========== HDMI In APIs Testing Completed ==========");
        LOGINFO("- GetVRRSupport");
        LOGINFO("- GetVRRStatus");

        LOGINFO("========== HDMI In API Framework Ready ==========");

        // Test all HDMI In methods with sample values
        using HDMIInPort = DeviceSettingsHDMIIn::HDMIInPort;
        //using HDMIInSignalStatus = DeviceSettingsHDMIIn::HDMIInSignalStatus;
        //using HDMIVideoPortResolution = DeviceSettingsHDMIIn::HDMIVideoPortResolution;
        //using HDMIInAviContentType = DeviceSettingsHDMIIn::HDMIInAviContentType;

        // Test sample ports
        HDMIInPort testPorts[] = {
            HDMIInPort::DS_HDMI_IN_PORT_0,
            HDMIInPort::DS_HDMI_IN_PORT_1,
            HDMIInPort::DS_HDMI_IN_PORT_2
        };

        // Use HDMI In Manager interface directly
        DeviceSettingsHDMIIn* hdmiIn = _hdmiInManager;

        result = 0;
        for (auto port : testPorts) {
            LOGINFO("---------- Testing HDMI In Port: %d ----------", static_cast<int>(port));

            // 1. Test GetHDMIInNumbefOfInputs
            int32_t numInputs = 0;
            result = hdmiIn->GetHDMIInNumbefOfInputs(numInputs);
            LOGINFO("GetHDMIInNumbefOfInputs: result=%u, numInputs=%d", result, numInputs);

            // 2. Test GetHDMIInStatus
            DeviceSettingsHDMIIn::HDMIInStatus hdmiStatus;
            DeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator* portConnIter = nullptr;
            result = hdmiIn->GetHDMIInStatus(hdmiStatus, portConnIter);
            LOGINFO("GetHDMIInStatus: result=%u, isPresented=%d, activePort=%d", result, hdmiStatus.isPresented, static_cast<int>(hdmiStatus.activePort));
            if (portConnIter) portConnIter->Release();

            // 3. Test SelectHDMIInPort with get-set-restore pattern
            DeviceSettingsHDMIIn::HDMIInPort originalActivePort = hdmiStatus.activePort; // Save original active port
            result = hdmiIn->SelectHDMIInPort(port, true, true, DeviceSettingsHDMIIn::HDMIVideoPlaneType::DS_HDMIIN_VIDEOPLANE_PRIMARY);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SelectHDMIInPort completed");
            } else {
                LOGERR("FAILED SelectHDMIInPort call");
            }

            // Restore original active port
            result = hdmiIn->SelectHDMIInPort(originalActivePort, true, true, DeviceSettingsHDMIIn::HDMIVideoPlaneType::DS_HDMIIN_VIDEOPLANE_PRIMARY);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS HDMI port selection restored");
            } else {
                LOGERR("FAILED HDMI port selection restore");
            }

            // 4. Test ScaleHDMIInVideo with get-set-restore pattern
            DeviceSettingsHDMIIn::HDMIInVideoRectangle testRect = {100, 100, 1920, 1080};
            result = hdmiIn->ScaleHDMIInVideo(testRect);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS ScaleHDMIInVideo completed");
            } else {
                LOGERR("FAILED ScaleHDMIInVideo call");
            }

            // Restore to full screen (assuming default)
            DeviceSettingsHDMIIn::HDMIInVideoRectangle defaultRect = {0, 0, 1920, 1080};
            result = hdmiIn->ScaleHDMIInVideo(defaultRect);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS HDMI video scaling restored");
            } else {
                LOGERR("FAILED HDMI video scaling restore");
            }

            // 5. Test GetSupportedGameFeaturesList
            DeviceSettingsHDMIIn::IHDMIInGameFeatureListIterator* gameFeatureList = nullptr;
            result = hdmiIn->GetSupportedGameFeaturesList(gameFeatureList);
            LOGINFO("GetSupportedGameFeaturesList: result=%u", result);

            // Print all game features from the iterator
            if (gameFeatureList && result == Core::ERROR_NONE) {
                LOGINFO("Printing game features from iterator:");
                DeviceSettingsHDMIIn::HDMIInGameFeatureList currentFeature;
                uint32_t featureIndex = 0;

                // Iterate through all features (iterator starts at beginning by default)
                bool hasMore = true;
                while (hasMore) {
                    hasMore = gameFeatureList->Next(currentFeature);
                    if (hasMore) {
                        LOGINFO("  GameFeature[%u]: '%s'", featureIndex, currentFeature.gameFeature.c_str());
                        featureIndex++;
                    }
                }

                LOGINFO("Total game features found: %u", featureIndex);
                gameFeatureList->Release();
            } else if (gameFeatureList) {
                LOGERR("GetSupportedGameFeaturesList failed with result=%u", result);
                gameFeatureList->Release();
            } else {
                LOGERR("GetSupportedGameFeaturesList returned null iterator");
            }

            // 6. Test GetHDMIInAVLatency
            uint32_t videoLatency = 0, audioLatency = 0;
            result = hdmiIn->GetHDMIInAVLatency(videoLatency, audioLatency);
            LOGINFO("GetHDMIInAVLatency: result=%u, videoLatency=%u, audioLatency=%u", result, videoLatency, audioLatency);

            // 7. Test GetHDMIVideoMode
            DeviceSettingsHDMIIn::HDMIVideoPortResolution videoMode;
            result = hdmiIn->GetHDMIVideoMode(videoMode);
            LOGINFO("GetHDMIVideoMode: result=%u, name=%s", result, videoMode.name.c_str());

            // 8. Test GetHDMIVersion
            DeviceSettingsHDMIIn::HDMIInCapabilityVersion capVersion;
            result = hdmiIn->GetHDMIVersion(port, capVersion);
            LOGINFO("GetHDMIVersion: result=%u, version=%d", result, static_cast<int>(capVersion));

            // 9. Test GetEdidBytes
            uint8_t edidBytes[256] = {0};
            result = hdmiIn->GetEdidBytes(port, 256, edidBytes);
            LOGINFO("GetEdidBytes: result=%u", result);

            // 10. Test GetHDMISPDInformation
            uint8_t spdBytes[256] = {0};
            result = hdmiIn->GetHDMISPDInformation(port, 256, spdBytes);
            LOGINFO("GetHDMISPDInformation: result=%u", result);

            // 11. Test GetHDMIEdidVersion
            DeviceSettingsHDMIIn::HDMIInEdidVersion edidVersion;
            result = hdmiIn->GetHDMIEdidVersion(port, edidVersion);
            LOGINFO("GetHDMIEdidVersion: result=%u, version=%d", result, static_cast<int>(edidVersion));

            // 12. Test SetHDMIEdidVersion with get-set-restore pattern
            DeviceSettingsHDMIIn::HDMIInEdidVersion originalEdidVersion = edidVersion; // Save original
            DeviceSettingsHDMIIn::HDMIInEdidVersion newEdidVersion = (edidVersion == DeviceSettingsHDMIIn::HDMIInEdidVersion::HDMI_EDID_VER_14) ? 
                DeviceSettingsHDMIIn::HDMIInEdidVersion::HDMI_EDID_VER_20 : DeviceSettingsHDMIIn::HDMIInEdidVersion::HDMI_EDID_VER_14;
            result = hdmiIn->SetHDMIEdidVersion(port, newEdidVersion);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SetHDMIEdidVersion completed");
            } else {
                LOGERR("FAILED SetHDMIEdidVersion call");
            }

            // Restore original EDID version
            result = hdmiIn->SetHDMIEdidVersion(port, originalEdidVersion);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS HDMI EDID version restored");
            } else {
                LOGERR("FAILED HDMI EDID version restore");
            }

            // 13. Test GetHDMIInAllmStatus
            bool allmStatus = false;
            result = hdmiIn->GetHDMIInAllmStatus(port, allmStatus);
            LOGINFO("GetHDMIInAllmStatus: result=%u, allmStatus=%d", result, allmStatus);

            // 14. Test GetHDMIInEdid2AllmSupport
            bool allmSupport = false;
            result = hdmiIn->GetHDMIInEdid2AllmSupport(port, allmSupport);
            LOGINFO("GetHDMIInEdid2AllmSupport: result=%u, allmSupport=%d", result, allmSupport);

            // 15. Test SetHDMIInEdid2AllmSupport with get-set-restore pattern
            bool originalAllmSupport = allmSupport; // Save original
            bool newAllmSupport = !allmSupport; // Use opposite value
            result = hdmiIn->SetHDMIInEdid2AllmSupport(port, newAllmSupport);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SetHDMIInEdid2AllmSupport completed");
            } else {
                LOGERR("FAILED SetHDMIInEdid2AllmSupport call");
            }

            // Restore original ALLM support
            result = hdmiIn->SetHDMIInEdid2AllmSupport(port, originalAllmSupport);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS HDMI ALLM support restored");
            } else {
                LOGERR("FAILED HDMI ALLM support restore");
            }

            // 16. Test SelectHDMIZoomMode with get-set-restore pattern
            result = hdmiIn->SelectHDMIZoomMode(DeviceSettingsHDMIIn::HDMIInVideoZoom::DS_HDMIIN_VIDEO_ZOOM_FULL);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SelectHDMIZoomMode completed");
            } else {
                LOGERR("FAILED SelectHDMIZoomMode call");
            }

            // Restore to default zoom mode (assuming NONE is default)
            result = hdmiIn->SelectHDMIZoomMode(DeviceSettingsHDMIIn::HDMIInVideoZoom::DS_HDMIIN_VIDEO_ZOOM_NONE);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS HDMI zoom mode restored");
            } else {
                LOGERR("FAILED HDMI zoom mode restore");
            }

            // 17. Test GetVRRSupport
            bool vrrSupport = false;
            result = hdmiIn->GetVRRSupport(port, vrrSupport);
            LOGINFO("GetVRRSupport: result=%u, vrrSupport=%d", result, vrrSupport);

            // 18. Test SetVRRSupport with get-set-restore pattern
            bool originalVrrSupport = vrrSupport; // Save original
            bool newVrrSupport = !vrrSupport; // Use opposite value
            result = hdmiIn->SetVRRSupport(port, newVrrSupport);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SetVRRSupport completed");
            } else {
                LOGERR("FAILED SetVRRSupport call");
            }

            // Restore original VRR support
            result = hdmiIn->SetVRRSupport(port, originalVrrSupport);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS HDMI VRR support restored");
            } else {
                LOGERR("FAILED HDMI VRR support restore");
            }

            // 19. Test GetVRRStatus
            DeviceSettingsHDMIIn::HDMIInVRRStatus vrrStatus;
            result = hdmiIn->GetVRRStatus(port, vrrStatus);
            LOGINFO("GetVRRStatus: result=%u, vrrType=%d, framerate=%.2f", result, static_cast<int>(vrrStatus.vrrType), vrrStatus.vrrFreeSyncFramerateHz);

            LOGINFO("---------- Completed testing HDMI In Port: %d ----------", static_cast<int>(port));
        }

        // Test some additional methods that don't require specific ports
        LOGINFO("---------- Testing Additional HDMI In Methods ----------");

        // Test with sample resolution settings
        DeviceSettingsHDMIIn::HDMIVideoPortResolution testResolution;
        testResolution.name = "1920x1080p60";
        LOGINFO("Testing with resolution: %s", testResolution.name.c_str());

        // No need to release _hdmiInManager as it's managed by the class

        LOGINFO("========== HDMI In Methods Testing Completed ==========");
    }

    // HDMI In Event Handler Implementations
    void UserPlugin::OnHDMIInEventHotPlug(const Exchange::IDeviceSettingsHDMIIn::HDMIInPort port, const bool isConnected)
    {
        LOGINFO("========== HDMI In Event: Hot Plug ==========");
        LOGINFO("OnHDMIInEventHotPlug: port=%d, isConnected=%s", static_cast<int>(port), isConnected ? "true" : "false");

        // Add your custom logic here
        if (isConnected) {
            LOGINFO("HDMI device connected on port %d", static_cast<int>(port));
        } else {
            LOGINFO("HDMI device disconnected from port %d", static_cast<int>(port));
        }
    }

    void UserPlugin::OnHDMIInEventSignalStatus(const Exchange::IDeviceSettingsHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsHDMIIn::HDMIInSignalStatus signalStatus)
    {
        LOGINFO("========== HDMI In Event: Signal Status ==========");
        LOGINFO("OnHDMIInEventSignalStatus: port=%d, signalStatus=%d", static_cast<int>(port), static_cast<int>(signalStatus));

        // Add your custom logic here - using generic logging since exact enum values may vary
        LOGINFO("Signal status changed to %d on port %d", static_cast<int>(signalStatus), static_cast<int>(port));

        // Add specific handling based on your interface definition
        if (static_cast<int>(signalStatus) == 0) {
            LOGINFO("No signal detected on port %d", static_cast<int>(port));
        } else if (static_cast<int>(signalStatus) == 1) {
            LOGINFO("Stable signal detected on port %d", static_cast<int>(port));
        } else if (static_cast<int>(signalStatus) == 2) {
            LOGINFO("Unstable signal detected on port %d", static_cast<int>(port));
        } else {
            LOGINFO("Unknown signal status %d on port %d", static_cast<int>(signalStatus), static_cast<int>(port));
        }
    }

    void UserPlugin::OnHDMIInEventStatus(const Exchange::IDeviceSettingsHDMIIn::HDMIInPort activePort, const bool isPresented)
    {
        LOGINFO("========== HDMI In Event: Status ==========");
        LOGINFO("OnHDMIInEventStatus: activePort=%d, isPresented=%s", static_cast<int>(activePort), isPresented ? "true" : "false");

        // Add your custom logic here
        if (isPresented) {
            LOGINFO("HDMI input is being presented on port %d", static_cast<int>(activePort));
        } else {
            LOGINFO("HDMI input is not being presented on port %d", static_cast<int>(activePort));
        }
    }

    void UserPlugin::OnHDMIInVideoModeUpdate(const Exchange::IDeviceSettingsHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsHDMIIn::HDMIVideoPortResolution videoPortResolution)
    {
        LOGINFO("========== HDMI In Event: Video Mode Update ==========");
        LOGINFO("OnHDMIInVideoModeUpdate: port=%d", static_cast<int>(port));
        LOGINFO("Video resolution: name='%s', pixelResolution=%d, aspectRatio=%d, stereoScopicMode=%d, frameRate=%d, interlaced=%s", 
                videoPortResolution.name.c_str(),
                static_cast<int>(videoPortResolution.pixelResolution),
                static_cast<int>(videoPortResolution.aspectRatio),
                static_cast<int>(videoPortResolution.stereoScopicMode),
                static_cast<int>(videoPortResolution.frameRate),
                videoPortResolution.interlaced ? "true" : "false");

        // Add your custom logic here
        LOGINFO("Video mode changed on port %d to %s", static_cast<int>(port), videoPortResolution.name.c_str());
    }

    void UserPlugin::OnHDMIInAllmStatus(const Exchange::IDeviceSettingsHDMIIn::HDMIInPort port, const bool allmStatus)
    {
        LOGINFO("========== HDMI In Event: ALLM Status ==========");
        LOGINFO("OnHDMIInAllmStatus: port=%d, allmStatus=%s", static_cast<int>(port), allmStatus ? "enabled" : "disabled");

        // Add your custom logic here
        if (allmStatus) {
            LOGINFO("Auto Low Latency Mode (ALLM) enabled on port %d", static_cast<int>(port));
        } else {
            LOGINFO("Auto Low Latency Mode (ALLM) disabled on port %d", static_cast<int>(port));
        }
    }

    void UserPlugin::OnHDMIInAVIContentType(const Exchange::IDeviceSettingsHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsHDMIIn::HDMIInAviContentType aviContentType)
    {
        LOGINFO("========== HDMI In Event: AVI Content Type ==========");
        LOGINFO("OnHDMIInAVIContentType: port=%d, aviContentType=%d", static_cast<int>(port), static_cast<int>(aviContentType));

        // Add your custom logic here - using generic logging since exact enum values may vary
        LOGINFO("AVI Content Type changed to %d on port %d", static_cast<int>(aviContentType), static_cast<int>(port));

        // Add specific handling based on your interface definition
        if (static_cast<int>(aviContentType) == 0) {
            LOGINFO("AVI Content Type: Graphics on port %d", static_cast<int>(port));
        } else if (static_cast<int>(aviContentType) == 1) {
            LOGINFO("AVI Content Type: Photo on port %d", static_cast<int>(port));
        } else if (static_cast<int>(aviContentType) == 2) {
            LOGINFO("AVI Content Type: Cinema on port %d", static_cast<int>(port));
        } else if (static_cast<int>(aviContentType) == 3) {
            LOGINFO("AVI Content Type: Game on port %d", static_cast<int>(port));
        } else {
            LOGINFO("AVI Content Type: Unknown (%d) on port %d", static_cast<int>(aviContentType), static_cast<int>(port));
        }
    }

    void UserPlugin::OnHDMIInAVLatency(const int32_t audioDelay, const int32_t videoDelay)
    {
        LOGINFO("========== HDMI In Event: AV Latency ==========");
        LOGINFO("OnHDMIInAVLatency: audioDelay=%d ms, videoDelay=%d ms", audioDelay, videoDelay);

        // Add your custom logic here
        LOGINFO("Audio/Video latency updated - Audio: %d ms, Video: %d ms", audioDelay, videoDelay);

        if (audioDelay != videoDelay) {
            LOGINFO("Audio/Video sync required - difference: %d ms", abs(audioDelay - videoDelay));
        } else {
            LOGINFO("Audio/Video are in sync");
        }
    }

    void UserPlugin::OnHDMIInVRRStatus(const Exchange::IDeviceSettingsHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsHDMIIn::HDMIInVRRType vrrType)
    {
        LOGINFO("========== HDMI In Event: VRR Status ==========");
        LOGINFO("OnHDMIInVRRStatus: port=%d, vrrType=%d", static_cast<int>(port), static_cast<int>(vrrType));

        // Add your custom logic here - using generic logging since exact enum values may vary
        LOGINFO("VRR Type changed to %d on port %d", static_cast<int>(vrrType), static_cast<int>(port));

        // Add specific handling based on your interface definition
        if (static_cast<int>(vrrType) == 0) {
            LOGINFO("VRR Type: None on port %d", static_cast<int>(port));
        } else if (static_cast<int>(vrrType) == 1) {
            LOGINFO("VRR Type: FreeSync on port %d", static_cast<int>(port));
        } else if (static_cast<int>(vrrType) == 2) {
            LOGINFO("VRR Type: VRR on port %d", static_cast<int>(port));
        } else {
            LOGINFO("VRR Type: Unknown (%d) on port %d", static_cast<int>(vrrType), static_cast<int>(port));
        }
    }

    // IARM API implementations for direct DsMgr daemon communication
    Core::hresult UserPlugin::TestIARMHdmiInSelectPort(const int port, const bool requestAudioMix, const bool topMostPlane, const int videoPlaneType)
    {
        dsHdmiInSelectPortParam_t param;
        memset(&param, 0, sizeof(param));

        // Set parameters and call IARM
        param.port = static_cast<dsHdmiInPort_t>(port);
        param.requestAudioMix = requestAudioMix;
        param.topMostPlane = topMostPlane;
        param.videoPlaneType = static_cast<dsVideoPlaneType_t>(videoPlaneType);

        IARM_Result_t result = IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsHdmiInSelectPort, &param, sizeof(param));

        if (result == IARM_RESULT_SUCCESS && param.result == dsERR_NONE) {
            LOGINFO("SUCCESS dsHdmiInSelectPort completed");
            return Core::ERROR_NONE;
        } else {
            LOGERR("FAILED dsHdmiInSelectPort call");
            return Core::ERROR_GENERAL;
        }
    }

    Core::hresult UserPlugin::TestIARMHdmiInScaleVideo(const int x, const int y, const int width, const int height)
    {
        dsHdmiInScaleVideoParam_t param;
        memset(&param, 0, sizeof(param));

        // Set video rectangle parameters and call IARM
        param.videoRect.x = x;
        param.videoRect.y = y;
        param.videoRect.width = width;
        param.videoRect.height = height;

        IARM_Result_t result = IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsHdmiInScaleVideo, &param, sizeof(param));

        if (result == IARM_RESULT_SUCCESS && param.result == dsERR_NONE) {
            LOGINFO("SUCCESS dsHdmiInScaleVideo completed");
            return Core::ERROR_NONE;
        } else {
            LOGERR("FAILED dsHdmiInScaleVideo call");
            return Core::ERROR_GENERAL;
        }
    }

    void UserPlugin::TestIARMHdmiInAPIs()
    {
        LOGINFO("=== IARM Basic HDMI Tests ===");

        // Test port selection
        TestIARMHdmiInSelectPort(0, true, true, 0);
        sleep(1);
        TestIARMHdmiInSelectPort(1, false, false, 1);
        sleep(1);

        // Test video scaling
        TestIARMHdmiInScaleVideo(0, 0, 1920, 1080);
        sleep(1);
        TestIARMHdmiInScaleVideo(480, 270, 960, 540);
        sleep(1);
        TestIARMHdmiInScaleVideo(100, 100, 640, 360);

        LOGINFO("=== IARM Basic Tests Complete ===");
    }

    // Additional IARM API implementations for advanced DsMgr daemon functionality
    Core::hresult UserPlugin::TestIARMGetAVLatency()
    {
        dsTVAudioVideoLatencyParam_t param;
        memset(&param, 0, sizeof(param));

        IARM_Result_t result = IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsGetAVLatency, &param, sizeof(param));

        if (result == IARM_RESULT_SUCCESS && param.result == dsERR_NONE) {
            LOGINFO("SUCCESS dsGetAVLatency completed");
            return Core::ERROR_NONE;
        } else {
            LOGERR("FAILED dsGetAVLatency call");
            return Core::ERROR_GENERAL;
        }
    }

    Core::hresult UserPlugin::TestIARMGetAllmStatus(const int port)
    {
        dsAllmStatusParam_t param;
        memset(&param, 0, sizeof(param));
        param.iHdmiPort = static_cast<dsHdmiInPort_t>(port);

        IARM_Result_t result = IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsGetAllmStatus, &param, sizeof(param));

        if (result == IARM_RESULT_SUCCESS && param.result == dsERR_NONE) {
            LOGINFO("SUCCESS dsGetAllmStatus Port complete");
            return Core::ERROR_NONE;
        } else {
            LOGERR("FAILED dsGetAllmStatus Port call");
            return Core::ERROR_GENERAL;
        }
    }

    Core::hresult UserPlugin::TestIARMSetEdid2AllmSupport(const int port, const bool allmSupport)
    {
        // Step 1: Get original ALLM support value
        dsEdidAllmSupportParam_t getParam;
        memset(&getParam, 0, sizeof(getParam));
        getParam.iHdmiPort = static_cast<dsHdmiInPort_t>(port);

        IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsGetAllmStatus, &getParam, sizeof(getParam));
        bool originalValue = getParam.allmSupport;

        // Step 2: Set new value
        dsEdidAllmSupportParam_t setParam;
        memset(&setParam, 0, sizeof(setParam));
        setParam.iHdmiPort = static_cast<dsHdmiInPort_t>(port);
        setParam.allmSupport = allmSupport;

        IARM_Result_t setResult = IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsSetEdid2AllmSupport, &setParam, sizeof(setParam));

        // Step 3: Restore original value
        dsEdidAllmSupportParam_t restoreParam;
        memset(&restoreParam, 0, sizeof(restoreParam));
        restoreParam.iHdmiPort = static_cast<dsHdmiInPort_t>(port);
        restoreParam.allmSupport = originalValue;
        IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsSetEdid2AllmSupport, &restoreParam, sizeof(restoreParam));

        if (setResult == IARM_RESULT_SUCCESS && setParam.result == dsERR_NONE) {
            LOGINFO("SUCCESS dsSetEdid2AllmSupport completed");
            return Core::ERROR_NONE;
        } else {
            LOGERR("FAILED dsSetEdid2AllmSupport call");
            return Core::ERROR_GENERAL;
        }
    }

    Core::hresult UserPlugin::TestIARMSetEdidVersion(const int port, const int edidVersion)
    {
        // Step 1: Get original EDID version (assume original = 1 if get fails)
        int originalVersion = 1;

        // Step 2: Set new version
        dsEdidVersionParam_t setParam;
        memset(&setParam, 0, sizeof(setParam));
        setParam.iHdmiPort = static_cast<dsHdmiInPort_t>(port);
        setParam.iEdidVersion = static_cast<tv_hdmi_edid_version_t>(edidVersion);

        IARM_Result_t setResult = IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsSetEdidVersion, &setParam, sizeof(setParam));

        // Step 3: Restore original version
        dsEdidVersionParam_t restoreParam;
        memset(&restoreParam, 0, sizeof(restoreParam));
        restoreParam.iHdmiPort = static_cast<dsHdmiInPort_t>(port);
        restoreParam.iEdidVersion = static_cast<tv_hdmi_edid_version_t>(originalVersion);
        IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsSetEdidVersion, &restoreParam, sizeof(restoreParam));

        if (setResult == IARM_RESULT_SUCCESS && setParam.result == dsERR_NONE) {
            LOGINFO("SUCCESS dsSetEdidVersion completed");
            return Core::ERROR_NONE;
        } else {
            LOGERR("FAILED dsSetEdidVersion call");
            return Core::ERROR_GENERAL;
        }
    }

    Core::hresult UserPlugin::TestIARMHdmiInGetCurrentVideoMode()
    {
        dsVideoPortResolution_t resolution;
        memset(&resolution, 0, sizeof(resolution));

        IARM_Result_t result = IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsHdmiInGetCurrentVideoMode, &resolution, sizeof(resolution));

        if (result == IARM_RESULT_SUCCESS) {
            LOGINFO("SUCCESS dsHdmiInGetCurrentVideoMode completed");
            return Core::ERROR_NONE;
        } else {
            LOGERR("FAILED dsHdmiInGetCurrentVideoMode call");
            return Core::ERROR_GENERAL;
        }
    }

    Core::hresult UserPlugin::TestIARMSetVRRSupport(const int port, const bool vrrSupport)
    {
        // Step 1: Get original VRR support value
        dsVRRSupportParam_t getParam;
        memset(&getParam, 0, sizeof(getParam));
        getParam.iHdmiPort = static_cast<dsHdmiInPort_t>(port);

        IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsGetVRRSupport, &getParam, sizeof(getParam));
        bool originalValue = getParam.vrrSupport;

        // Step 2: Set new value
        dsVRRSupportParam_t setParam;
        memset(&setParam, 0, sizeof(setParam));
        setParam.iHdmiPort = static_cast<dsHdmiInPort_t>(port);
        setParam.vrrSupport = vrrSupport;

        IARM_Result_t setResult = IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsSetVRRSupport, &setParam, sizeof(setParam));

        // Step 3: Restore original value
        dsVRRSupportParam_t restoreParam;
        memset(&restoreParam, 0, sizeof(restoreParam));
        restoreParam.iHdmiPort = static_cast<dsHdmiInPort_t>(port);
        restoreParam.vrrSupport = originalValue;
        IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsSetVRRSupport, &restoreParam, sizeof(restoreParam));

        if (setResult == IARM_RESULT_SUCCESS && setParam.result == dsERR_NONE) {
            LOGINFO("SUCCESS dsSetVRRSupport completed");
            return Core::ERROR_NONE;
        } else {
            LOGERR("FAILED dsSetVRRSupport call");
            return Core::ERROR_GENERAL;
        }
    }

    Core::hresult UserPlugin::TestIARMGetVRRSupport(const int port)
    {
        dsVRRSupportParam_t param;
        memset(&param, 0, sizeof(param));
        param.iHdmiPort = static_cast<dsHdmiInPort_t>(port);

        IARM_Result_t result = IARM_Bus_Call(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_API_dsGetVRRSupport, &param, sizeof(param));

        if (result == IARM_RESULT_SUCCESS && param.result == dsERR_NONE) {
            LOGINFO("SUCCESS dsGetVRRSupport completed");
            return Core::ERROR_NONE;
        } else {
            LOGERR("FAILED dsGetVRRSupport call");
            return Core::ERROR_GENERAL;
        }
    }

    void UserPlugin::TestIARMAdditionalAPIs()
    {
        LOGINFO("=== IARM Advanced HDMI Tests ===");

        // Test AV Latency
        TestIARMGetAVLatency();
        sleep(1);

        // Test ALLM with get-set-restore pattern
        TestIARMGetAllmStatus(0);
        sleep(1);
        TestIARMSetEdid2AllmSupport(0, true);  // Will restore original value
        sleep(1);
        TestIARMGetAllmStatus(0);
        sleep(1);

        // Test EDID Version with get-set-restore pattern
        TestIARMSetEdidVersion(0, 1);  // Will restore original value
        sleep(1);
        TestIARMSetEdidVersion(1, 2);  // Will restore original value
        sleep(1);

        // Test Video Mode Info
        TestIARMHdmiInGetCurrentVideoMode();
        sleep(1);

        // Test VRR with get-set-restore pattern
        TestIARMGetVRRSupport(0);
        sleep(1);
        TestIARMSetVRRSupport(0, true);  // Will restore original value
        sleep(1);
        TestIARMGetVRRSupport(0);
        sleep(1);
        TestIARMSetVRRSupport(1, true);  // Will restore original value
        sleep(1);
        TestIARMGetVRRSupport(1);

        LOGINFO("=== IARM Advanced Tests Complete ===");
    }

} // namespace Plugin
} // namespace WPEFramework
