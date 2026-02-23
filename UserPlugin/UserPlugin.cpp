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
        TestSimplifiedFPDAPIs();
        TestSimplifiedHDMIInAPIs();
        TestSelectHDMIInPortAPI();

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

    void UserPlugin::TestSimplifiedFPDAPIs()
    {
        LOGINFO("========== Simplified FPD APIs Testing Framework ==========");

        if (!_fpdManager) {
            LOGERR("FPD Manager interface is not available");
            return;
        }

        LOGINFO("Testing only 4 FPD APIs: SetFPDBrightness, GetFPDBrightness, SetFPDState, GetFPDState");
        LOGINFO("Testing only POWER indicator");

        // Test FPD indicators - only POWER indicator
        Exchange::IDeviceSettingsFPD::FPDIndicator testIndicators[] = {
            Exchange::IDeviceSettingsFPD::FPDIndicator::DS_FPD_INDICATOR_POWER
        };

        const char* indicatorNames[] = {
            "POWER"
        };

        // Test the 4 specified FPD APIs for each indicator
        for (size_t i = 0; i < sizeof(testIndicators)/sizeof(testIndicators[0]); i++) {
            auto indicator = testIndicators[i];
            const char* indicatorName = indicatorNames[i];

            LOGINFO("---------- Testing FPD Indicator: %s ----------", indicatorName);

            // 1. GetFPDBrightness - Get current brightness value
            uint32_t originalBrightness = 0;
            Core::hresult result = _fpdManager->GetFPDBrightness(indicator, originalBrightness);
            LOGINFO("GetFPDBrightness: indicator=%s, result=%u, original_brightness=%u", indicatorName, result, originalBrightness);

            if (result == Core::ERROR_NONE) {
                // 2. SetFPDBrightness - Set test brightness value
                uint32_t testBrightness = (originalBrightness == 50) ? 75 : 50;
                result = _fpdManager->SetFPDBrightness(indicator, testBrightness, false);
                LOGINFO("SetFPDBrightness: indicator=%s, result=%u, test_brightness=%u", indicatorName, result, testBrightness);

                // Verify the brightness was set correctly
                uint32_t verifyBrightness = 0;
                _fpdManager->GetFPDBrightness(indicator, verifyBrightness);
                LOGINFO("GetFPDBrightness: indicator=%s, verified_brightness=%u", indicatorName, verifyBrightness);

                // Restore original brightness
                _fpdManager->SetFPDBrightness(indicator, originalBrightness, false);
                LOGINFO("SetFPDBrightness: indicator=%s, brightness restored to %u", indicatorName, originalBrightness);
            }

            // 3. GetFPDState - Get current state value
            Exchange::IDeviceSettingsFPD::FPDState originalState;
            result = _fpdManager->GetFPDState(indicator, originalState);
            LOGINFO("GetFPDState: indicator=%s, result=%u, original_state=%u", indicatorName, result, (uint32_t)originalState);

            if (result == Core::ERROR_NONE) {
                // 4. SetFPDState - Set test state value
                Exchange::IDeviceSettingsFPD::FPDState testState = (originalState == Exchange::IDeviceSettingsFPD::FPDState::DS_FPD_STATE_ON) ? 
                    Exchange::IDeviceSettingsFPD::FPDState::DS_FPD_STATE_OFF : Exchange::IDeviceSettingsFPD::FPDState::DS_FPD_STATE_ON;
                result = _fpdManager->SetFPDState(indicator, testState);
                LOGINFO("SetFPDState: indicator=%s, result=%u, test_state=%u", indicatorName, result, (uint32_t)testState);

                // Verify the state was set correctly
                Exchange::IDeviceSettingsFPD::FPDState verifyState;
                _fpdManager->GetFPDState(indicator, verifyState);
                LOGINFO("GetFPDState: indicator=%s, verified_state=%u", indicatorName, (uint32_t)verifyState);

                // Restore original state
                _fpdManager->SetFPDState(indicator, originalState);
                LOGINFO("SetFPDState: indicator=%s, state restored to %u", indicatorName, (uint32_t)originalState);
            }
        }

        LOGINFO("========== Simplified FPD APIs Testing Completed ==========\\n");
    }

    void UserPlugin::TestSimplifiedHDMIInAPIs()
    {
        LOGINFO("========== Complete HDMI In APIs Testing Framework ==========");

        if (!_hdmiInManager) {
            LOGERR("HDMI In Manager interface is not available");
            return;
        }

        LOGINFO("Testing ALL HDMI In APIs with get-set-restore pattern");

        // Test ports - comprehensive testing on multiple ports
        DeviceSettingsHDMIIn::HDMIInPort testPorts[] = {
            DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_0,
            DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_1,
            DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_2
        };

        // 1. Test GetHDMIInNumbefOfInputs
        int32_t inputCount = 0;
        Core::hresult result = _hdmiInManager->GetHDMIInNumbefOfInputs(inputCount);
        LOGINFO("GetHDMIInNumbefOfInputs: result=%u, count=%d", result, inputCount);

        // 2. Test GetHDMIInStatus
        DeviceSettingsHDMIIn::HDMIInStatus hdmiStatus;
        DeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator* portStatus = nullptr;
        result = _hdmiInManager->GetHDMIInStatus(hdmiStatus, portStatus);
        LOGINFO("GetHDMIInStatus: result=%u, activePort=%d, isPresented=%s", 
               result, static_cast<int>(hdmiStatus.activePort), hdmiStatus.isPresented ? "true" : "false");
        if (portStatus) {
            portStatus->Release();
        }

        // 3. Test GetHDMIInAVLatency (global method)
        uint32_t videoLatency = 0, audioLatency = 0;
        result = _hdmiInManager->GetHDMIInAVLatency(videoLatency, audioLatency);
        LOGINFO("GetHDMIInAVLatency: result=%u, videoLatency=%u, audioLatency=%u", 
               result, videoLatency, audioLatency);

        // 4. Test GetHDMIVideoMode (global method)
        DeviceSettingsHDMIIn::HDMIVideoPortResolution videoMode;
        result = _hdmiInManager->GetHDMIVideoMode(videoMode);
        LOGINFO("GetHDMIVideoMode: result=%u, name='%s', pixelRes=%d, aspectRatio=%d, frameRate=%d", 
               result, videoMode.name.c_str(), static_cast<int>(videoMode.pixelResolution), 
               static_cast<int>(videoMode.aspectRatio), static_cast<int>(videoMode.frameRate));

        // 5. Test GetSupportedGameFeaturesList (global method)
        DeviceSettingsHDMIIn::IHDMIInGameFeatureListIterator* gameFeatures = nullptr;
        result = _hdmiInManager->GetSupportedGameFeaturesList(gameFeatures);
        LOGINFO("GetSupportedGameFeaturesList: result=%u", result);
        if (gameFeatures && result == Core::ERROR_NONE) {
            LOGINFO("Iterating through game features:");
            DeviceSettingsHDMIIn::HDMIInGameFeatureList currentFeature;
            uint32_t featureIndex = 0;
            bool hasMore = true;
            while (hasMore) {
                hasMore = gameFeatures->Next(currentFeature);
                if (hasMore) {
                    LOGINFO("  GameFeature[%u]: '%s'", featureIndex, currentFeature.gameFeature.c_str());
                    featureIndex++;
                }
            }
            LOGINFO("Total game features found: %u", featureIndex);
            gameFeatures->Release();
        } else if (gameFeatures) {
            gameFeatures->Release();
        }

        // Test each port with all port-specific APIs
        for (auto port : testPorts) {
            LOGINFO("---------- Testing ALL APIs for HDMI In Port: %d ----------", static_cast<int>(port));

            // 6. Test GetHDMIVersion (read-only per port)
            DeviceSettingsHDMIIn::HDMIInCapabilityVersion hdmiVersion;
            result = _hdmiInManager->GetHDMIVersion(port, hdmiVersion);
            LOGINFO("GetHDMIVersion: result=%u, port=%d, version=%d", result, static_cast<int>(port), static_cast<int>(hdmiVersion));

            // 7. Test GetHDMIEdidVersion and SetHDMIEdidVersion (get-set-restore)
            DeviceSettingsHDMIIn::HDMIInEdidVersion originalEdidVersion;
            result = _hdmiInManager->GetHDMIEdidVersion(port, originalEdidVersion);
            LOGINFO("GetHDMIEdidVersion: result=%u, port=%d, original_version=%d", result, static_cast<int>(port), static_cast<int>(originalEdidVersion));

            if (result == Core::ERROR_NONE) {
                // Set test EDID version (toggle between 1.4 and 2.0)
                DeviceSettingsHDMIIn::HDMIInEdidVersion testEdidVersion = (originalEdidVersion == DeviceSettingsHDMIIn::HDMIInEdidVersion::HDMI_EDID_VER_14) ? 
                    DeviceSettingsHDMIIn::HDMIInEdidVersion::HDMI_EDID_VER_20 : DeviceSettingsHDMIIn::HDMIInEdidVersion::HDMI_EDID_VER_14;
                result = _hdmiInManager->SetHDMIEdidVersion(port, testEdidVersion);
                LOGINFO("SetHDMIEdidVersion: result=%u, port=%d, test_version=%d", result, static_cast<int>(port), static_cast<int>(testEdidVersion));

                // Verify the version was set
                DeviceSettingsHDMIIn::HDMIInEdidVersion verifyEdidVersion;
                _hdmiInManager->GetHDMIEdidVersion(port, verifyEdidVersion);
                LOGINFO("GetHDMIEdidVersion: port=%d, verified_version=%d", static_cast<int>(port), static_cast<int>(verifyEdidVersion));

                // Restore original version
                _hdmiInManager->SetHDMIEdidVersion(port, originalEdidVersion);
                LOGINFO("SetHDMIEdidVersion: port=%d, version restored to %d", static_cast<int>(port), static_cast<int>(originalEdidVersion));
            }

            // 8. Test GetHDMIInEdid2AllmSupport and SetHDMIInEdid2AllmSupport (get-set-restore)
            bool originalAllmSupport = false;
            result = _hdmiInManager->GetHDMIInEdid2AllmSupport(port, originalAllmSupport);
            LOGINFO("GetHDMIInEdid2AllmSupport: result=%u, port=%d, original_support=%s", result, static_cast<int>(port), originalAllmSupport ? "true" : "false");

            if (result == Core::ERROR_NONE) {
                // Set test ALLM support (toggle value)
                bool testAllmSupport = !originalAllmSupport;
                result = _hdmiInManager->SetHDMIInEdid2AllmSupport(port, testAllmSupport);
                LOGINFO("SetHDMIInEdid2AllmSupport: result=%u, port=%d, test_support=%s", result, static_cast<int>(port), testAllmSupport ? "true" : "false");

                // Verify the support was set
                bool verifyAllmSupport = false;
                _hdmiInManager->GetHDMIInEdid2AllmSupport(port, verifyAllmSupport);
                LOGINFO("GetHDMIInEdid2AllmSupport: port=%d, verified_support=%s", static_cast<int>(port), verifyAllmSupport ? "true" : "false");

                // Restore original support
                _hdmiInManager->SetHDMIInEdid2AllmSupport(port, originalAllmSupport);
                LOGINFO("SetHDMIInEdid2AllmSupport: port=%d, support restored to %s", static_cast<int>(port), originalAllmSupport ? "true" : "false");
            }

            // 9. Test GetVRRSupport and SetVRRSupport (get-set-restore)
            bool originalVrrSupport = false;
            result = _hdmiInManager->GetVRRSupport(port, originalVrrSupport);
            LOGINFO("GetVRRSupport: result=%u, port=%d, original_support=%s", result, static_cast<int>(port), originalVrrSupport ? "true" : "false");

            if (result == Core::ERROR_NONE) {
                // Set test VRR support (toggle value)
                bool testVrrSupport = !originalVrrSupport;
                result = _hdmiInManager->SetVRRSupport(port, testVrrSupport);
                LOGINFO("SetVRRSupport: result=%u, port=%d, test_support=%s", result, static_cast<int>(port), testVrrSupport ? "true" : "false");

                // Verify the support was set
                bool verifyVrrSupport = false;
                _hdmiInManager->GetVRRSupport(port, verifyVrrSupport);
                LOGINFO("GetVRRSupport: port=%d, verified_support=%s", static_cast<int>(port), verifyVrrSupport ? "true" : "false");

                // Restore original support
                _hdmiInManager->SetVRRSupport(port, originalVrrSupport);
                LOGINFO("SetVRRSupport: port=%d, support restored to %s", static_cast<int>(port), originalVrrSupport ? "true" : "false");
            }

            // 10. Test GetVRRStatus (read-only)
            DeviceSettingsHDMIIn::HDMIInVRRStatus vrrStatus;
            result = _hdmiInManager->GetVRRStatus(port, vrrStatus);
            LOGINFO("GetVRRStatus: result=%u, port=%d, vrrType=%d, framerate=%.2f", 
                   result, static_cast<int>(port), static_cast<int>(vrrStatus.vrrType), vrrStatus.vrrFreeSyncFramerateHz);

            // 11. Test GetHDMIInAllmStatus (read-only)
            bool allmStatus = false;
            result = _hdmiInManager->GetHDMIInAllmStatus(port, allmStatus);
            LOGINFO("GetHDMIInAllmStatus: result=%u, port=%d, status=%s", result, static_cast<int>(port), allmStatus ? "true" : "false");

            // 12. Test GetEdidBytes (read-only byte array)
            uint8_t edidBytes[256] = {0};
            result = _hdmiInManager->GetEdidBytes(port, 256, edidBytes);
            LOGINFO("GetEdidBytes: result=%u, port=%d, bytes_length=256", result, static_cast<int>(port));
            if (result == Core::ERROR_NONE) {
                LOGINFO("EDID bytes retrieved successfully for Port %d", static_cast<int>(port));
                // Optionally log first few bytes for verification
                LOGINFO("First 8 EDID bytes: %02X %02X %02X %02X %02X %02X %02X %02X", 
                       edidBytes[0], edidBytes[1], edidBytes[2], edidBytes[3], 
                       edidBytes[4], edidBytes[5], edidBytes[6], edidBytes[7]);
            }

            // 13. Test GetHDMISPDInformation (read-only byte array)
            uint8_t spdBytes[256] = {0};
            result = _hdmiInManager->GetHDMISPDInformation(port, 256, spdBytes);
            LOGINFO("GetHDMISPDInformation: result=%u, port=%d, bytes_length=256", result, static_cast<int>(port));
            if (result == Core::ERROR_NONE) {
                LOGINFO("SPD information retrieved successfully for Port %d", static_cast<int>(port));
                // Optionally log first few bytes for verification
                LOGINFO("First 8 SPD bytes: %02X %02X %02X %02X %02X %02X %02X %02X", 
                       spdBytes[0], spdBytes[1], spdBytes[2], spdBytes[3], 
                       spdBytes[4], spdBytes[5], spdBytes[6], spdBytes[7]);
            }

            // 14. Test SelectHDMIInPort (get-set-restore with current status)
            DeviceSettingsHDMIIn::HDMIInStatus currentStatus;
            DeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator* currentPortStatus = nullptr;
            _hdmiInManager->GetHDMIInStatus(currentStatus, currentPortStatus);
            DeviceSettingsHDMIIn::HDMIInPort originalActivePort = currentStatus.activePort;
            if (currentPortStatus) {
                currentPortStatus->Release();
            }

            if (port != originalActivePort) {
                result = _hdmiInManager->SelectHDMIInPort(port, true, true, DeviceSettingsHDMIIn::HDMIVideoPlaneType::DS_HDMIIN_VIDEOPLANE_PRIMARY);
                LOGINFO("SelectHDMIInPort: result=%u, port=%d, audioMix=true, topMost=true, plane=PRIMARY", result, static_cast<int>(port));

                // Give device 20 seconds to switch ports
                LOGINFO("Waiting 20 seconds for device to switch to port %d...", static_cast<int>(port));
                sleep(20);

                // Verify port selection
                DeviceSettingsHDMIIn::HDMIInStatus verifyStatus;
                DeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator* verifyPortStatus = nullptr;
                _hdmiInManager->GetHDMIInStatus(verifyStatus, verifyPortStatus);
                LOGINFO("SelectHDMIInPort: verified_active_port=%d", static_cast<int>(verifyStatus.activePort));
                if (verifyPortStatus) {
                    verifyPortStatus->Release();
                }

                // Restore original active port
                _hdmiInManager->SelectHDMIInPort(originalActivePort, true, true, DeviceSettingsHDMIIn::HDMIVideoPlaneType::DS_HDMIIN_VIDEOPLANE_PRIMARY);
                LOGINFO("SelectHDMIInPort: port=%d, selection restored to %d", static_cast<int>(port), static_cast<int>(originalActivePort));

                // Give device 20 seconds to restore to original port
                LOGINFO("Waiting 20 seconds for device to restore to port %d...", static_cast<int>(originalActivePort));
                sleep(20);
            } else {
                LOGINFO("SelectHDMIInPort: port=%d is already active, skipping test", static_cast<int>(port));
            }

            // 15. Test ScaleHDMIInVideo (set-restore pattern)
            DeviceSettingsHDMIIn::HDMIInVideoRectangle testRect = {100, 100, 1280, 720};
            result = _hdmiInManager->ScaleHDMIInVideo(testRect);
            LOGINFO("ScaleHDMIInVideo: result=%u, port=%d, rect=(x=%d, y=%d, w=%d, h=%d)", 
                   result, static_cast<int>(port), testRect.x, testRect.y, testRect.width, testRect.height);

            // Restore to full screen (default)
            DeviceSettingsHDMIIn::HDMIInVideoRectangle defaultRect = {0, 0, 1920, 1080};
            result = _hdmiInManager->ScaleHDMIInVideo(defaultRect);
            LOGINFO("ScaleHDMIInVideo: port=%d, scaling restored to full screen", static_cast<int>(port));

            // 16. Test SelectHDMIZoomMode (set-restore pattern)
            result = _hdmiInManager->SelectHDMIZoomMode(DeviceSettingsHDMIIn::HDMIInVideoZoom::DS_HDMIIN_VIDEO_ZOOM_FULL);
            LOGINFO("SelectHDMIZoomMode: result=%u, port=%d, mode=FULL", result, static_cast<int>(port));

            // Restore to default zoom mode
            result = _hdmiInManager->SelectHDMIZoomMode(DeviceSettingsHDMIIn::HDMIInVideoZoom::DS_HDMIIN_VIDEO_ZOOM_NONE);
            LOGINFO("SelectHDMIZoomMode: port=%d, zoom mode restored to NONE", static_cast<int>(port));

            LOGINFO("---------- Completed ALL API testing for HDMI In Port: %d ----------", static_cast<int>(port));
        }

        LOGINFO("========== Complete HDMI In APIs Testing Completed ==========\\n");
    }

    void UserPlugin::TestSelectHDMIInPortAPI()
    {
        LOGINFO("========== Testing SelectHDMIInPort API for Port 2 ==========");

        if (!_hdmiInManager) {
            LOGERR("HDMI In Manager interface is not available");
            return;
        }

        // Get original active port
        DeviceSettingsHDMIIn::HDMIInStatus initialStatus;
        DeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator* initialPortStatus = nullptr;
        Core::hresult result = _hdmiInManager->GetHDMIInStatus(initialStatus, initialPortStatus);
        DeviceSettingsHDMIIn::HDMIInPort originalActivePort = initialStatus.activePort;
        LOGINFO("Original active port: %d", static_cast<int>(originalActivePort));
        if (initialPortStatus) {
            initialPortStatus->Release();
        }

        // Test switching to port 2
        DeviceSettingsHDMIIn::HDMIInPort targetPort = DeviceSettingsHDMIIn::HDMIInPort::DS_HDMI_IN_PORT_2;
        
        if (targetPort != originalActivePort) {
            LOGINFO("Switching to HDMI In Port 2...");
            result = _hdmiInManager->SelectHDMIInPort(targetPort, true, true, DeviceSettingsHDMIIn::HDMIVideoPlaneType::DS_HDMIIN_VIDEOPLANE_PRIMARY);
            LOGINFO("SelectHDMIInPort result: %u", result);

            if (result == Core::ERROR_NONE) {
                LOGINFO("Waiting 10 seconds for port switch...");
                sleep(10);

                // Verify port selection
                DeviceSettingsHDMIIn::HDMIInStatus verifyStatus;
                DeviceSettingsHDMIIn::IHDMIInPortConnectionStatusIterator* verifyPortStatus = nullptr;
                _hdmiInManager->GetHDMIInStatus(verifyStatus, verifyPortStatus);
                LOGINFO("Current active port after switch: %d", static_cast<int>(verifyStatus.activePort));
                if (verifyPortStatus) {
                    verifyPortStatus->Release();
                }

                // Restore to original port
                LOGINFO("Restoring to original port %d...", static_cast<int>(originalActivePort));
                _hdmiInManager->SelectHDMIInPort(originalActivePort, true, true, DeviceSettingsHDMIIn::HDMIVideoPlaneType::DS_HDMIIN_VIDEOPLANE_PRIMARY);
                sleep(10);
                LOGINFO("Port restored successfully");
            } else {
                LOGERR("Failed to switch to port 2, result: %u", result);
            }
        } else {
            LOGINFO("Port 2 is already active, no switching needed");
        }

        LOGINFO("========== SelectHDMIInPort API Test Completed ==========");
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

} // namespace Plugin
} // namespace WPEFramework
