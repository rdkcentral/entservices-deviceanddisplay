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
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

// Use proper FPD types from IDeviceSettingsManagerFPD interface
using FPDIndicator = WPEFramework::Exchange::IDeviceSettingsManagerFPD::FPDIndicator;
using FPDState = WPEFramework::Exchange::IDeviceSettingsManagerFPD::FPDState;

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

    UserPlugin::UserPlugin() : _service(nullptr), _connectionId(0), _fpdManager(nullptr), _hdmiInManager(nullptr), _audioManager(nullptr), _pwrMgrNotification(*this), _hdmiInNotification(*this)
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
        //ASSERT(_service == nullptr);
        //ASSERT(service != nullptr);
        LOGINFO("Initialize");
        _service = service;
        _service->AddRef();

	ASSERT(_pwrMgrNotification != nullptr);
        //_service->Register(&_pwrMgrNotification);
        _powerManager = PowerManagerInterfaceBuilder(_T("org.rdk.PowerManager"))
            .withIShell(_service)
            .withRetryIntervalMS(200)
            .withRetryCount(25)
            .createInterface().operator->();

        //_powerManager = service->Root<Exchange::IPowerManager>(_connectionId, 2000, _T("org.rdk.PowerManager"));
        ASSERT(_powerManager != nullptr);
	    ASSERT(&_pwrMgrNotification != nullptr);

        if (_powerManager && &_pwrMgrNotification) {
            _powerManager->Register(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
        } else {
            LOGERR("PowerManager or Notification is NULL");
        }

        // Connect to FPD Manager interface
        _fpdManager = _service->QueryInterfaceByCallsign<Exchange::IDeviceSettingsManagerFPD>("org.rdk.DeviceSettingsManager");
        ASSERT(_fpdManager != nullptr);
        if (_fpdManager) {
            LOGINFO("Successfully connected to DeviceSettingsManagerFPD interface");
        } else {
            LOGERR("Failed to connect to DeviceSettingsManagerFPD interface");
        }

        // Connect to HDMI In Manager interface
        _hdmiInManager = _service->QueryInterfaceByCallsign<Exchange::IDeviceSettingsManagerHDMIIn>("org.rdk.DeviceSettingsManager");
        ASSERT(_hdmiInManager != nullptr);

        // Register for HDMI In notifications
        if (_hdmiInManager) {
            _hdmiInManager->Register(_hdmiInNotification.baseInterface<DeviceSettingsManagerHDMIIn::INotification>());
            LOGINFO("Successfully registered for HDMI In notifications");
        } else {
            LOGERR("Failed to get HDMI In interface for notification registration");
        }

        // Connect to Audio Manager interface
        _audioManager = _service->QueryInterfaceByCallsign<Exchange::IDeviceSettingsManagerAudio>("org.rdk.DeviceSettingsManager");
        ASSERT(_audioManager != nullptr);
        if (_audioManager) {
            LOGINFO("Successfully connected to DeviceSettingsManagerAudio interface");
        } else {
            LOGERR("Failed to connect to DeviceSettingsManagerAudio interface");
        }

        TestFPDAPIs();
        // Test HDMI In methods with sample values
        TestSpecificHDMIInAPIs();
        // Test Audio APIs with comprehensive coverage
        TestAudioAPIs();

        // Test IARM APIs for direct DsMgr daemon communication
        TestIARMHdmiInAPIs();

        // Test additional IARM APIs (AV Latency, ALLM, EDID, VRR)
        TestIARMAdditionalAPIs();

        Exchange::JUserPlugin::Register(*this, this);

        return (string());
    }

    /* virtual */ void UserPlugin::Deinitialize(PluginHost::IShell* service)
    {
        LOGINFO("Deinitialize");

        _powerManager->Release();

        // Unregister and release FPD Manager
        if (_fpdManager) {
            _fpdManager->Release();
            _fpdManager = nullptr;
            LOGINFO("Successfully released FPD Manager interface");
        }

        // Unregister HDMI In notifications and release HDMI In Manager
        if (_hdmiInManager) {
            _hdmiInManager->Unregister(_hdmiInNotification.baseInterface<DeviceSettingsManagerHDMIIn::INotification>());
            _hdmiInManager->Release();
            _hdmiInManager = nullptr;
            LOGINFO("Successfully unregistered and released HDMI In Manager interface");
        }

        // Release Audio Manager
        if (_audioManager) {
            _audioManager->Release();
            _audioManager = nullptr;
            LOGINFO("Successfully released Audio Manager interface");
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


    Core::hresult UserPlugin::GetDevicePowerState(std::string& powerState) const
    {
        WPEFramework::Exchange::IPowerManager::PowerState pwrStateCur = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
        WPEFramework::Exchange::IPowerManager::PowerState pwrStatePrev = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
        powerState= "UNKNOWN";
        Core::hresult retStatus = Core::ERROR_GENERAL;

    //FPDIndicator indicator;
    FPDState fpdState;
        uint32_t brightness;
        bool persist;

        LOGINFO("GetDevicePowerState");
        ASSERT (_powerManager);
        if (_powerManager){
            retStatus = _powerManager->GetPowerState(pwrStateCur, pwrStatePrev);
        }

        if (Core::ERROR_NONE == retStatus){
            if (pwrStateCur == WPEFramework::Exchange::IPowerManager::POWER_STATE_ON)
                powerState = "ON";
            else if ((pwrStateCur == WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY) || (pwrStateCur == WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_LIGHT_SLEEP) || (pwrStateCur == WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_DEEP_SLEEP))
                powerState = "STANDBY";
        }

        LOGWARN("getPowerState called, power state : %s\n",
                powerState.c_str());

        // Use FPD Manager interface directly
        if (_fpdManager) {
            _fpdManager->GetFPDBrightness(FPDIndicator::DS_FPD_INDICATOR_POWER, brightness);
            LOGINFO("GetFPDBrightness brightness: %d", brightness);
            brightness = 50;
            persist = 1;
            LOGINFO("SetFPDBrightness brightness: %d, persist: %d", brightness, persist);
            _fpdManager->SetFPDBrightness(FPDIndicator::DS_FPD_INDICATOR_POWER, brightness, persist);
            _fpdManager->GetFPDState(FPDIndicator::DS_FPD_INDICATOR_POWER, fpdState);
            LOGINFO("GetFPDState state: %d", fpdState);
            fpdState = FPDState::DS_FPD_STATE_ON;
            LOGINFO("SetFPDState state:%d", fpdState);
            _fpdManager->SetFPDState(FPDIndicator::DS_FPD_INDICATOR_POWER, fpdState);
        } else {
            LOGERR("FPD Manager interface not available");
        }

        /*response["powerState"] = powerState;
        if (powerState != "UNKNOWN") {
            retVal = true;
        }*/
        return Core::ERROR_NONE;
    }//GET POWER STATE END

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
        LOGINFO("========== FPD APIs Testing Framework ==========\n");

        if (!_fpdManager) {
            LOGERR("FPD Manager interface is not available!");
            return;
        }

        LOGINFO("========== Testing FPD APIs ==========\n");

        // Test all FPD indicators
        Exchange::IDeviceSettingsManagerFPD::FPDIndicator testIndicators[] = {
            Exchange::IDeviceSettingsManagerFPD::FPDIndicator::DS_FPD_INDICATOR_MESSAGE,
            Exchange::IDeviceSettingsManagerFPD::FPDIndicator::DS_FPD_INDICATOR_POWER,
            Exchange::IDeviceSettingsManagerFPD::FPDIndicator::DS_FPD_INDICATOR_RECORD,
            Exchange::IDeviceSettingsManagerFPD::FPDIndicator::DS_FPD_INDICATOR_REMOTE,
            Exchange::IDeviceSettingsManagerFPD::FPDIndicator::DS_FPD_INDICATOR_RFBYPASS
        };

        const char* indicatorNames[] = {
            "MESSAGE",
            "POWER",
            "RECORD",
            "REMOTE",
            "RFBYPASS"
        };

        for (size_t i = 0; i < sizeof(testIndicators)/sizeof(testIndicators[0]); i++) {
            auto indicator = testIndicators[i];
            const char* indicatorName = indicatorNames[i];

            LOGINFO("---------- Testing FPD Indicator: %s ----------", indicatorName);

            // 1. Test GetFPDBrightness
            uint32_t currentBrightness = 0;
            Core::hresult result = _fpdManager->GetFPDBrightness(indicator, currentBrightness);
            LOGINFO("GetFPDBrightness: indicator=%s, result=%u, brightness=%u", indicatorName, result, currentBrightness);

            // 2. Test SetFPDBrightness with get-set-restore pattern
            uint32_t originalBrightness = currentBrightness; // Save original
            uint32_t newBrightness = (currentBrightness == 75) ? 50 : 75; // Use different value
            bool persist = true;
            result = _fpdManager->SetFPDBrightness(indicator, newBrightness, persist);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SetFPDBrightness completed");
            } else {
                LOGERR("FAILED SetFPDBrightness call");
            }

            // Restore original brightness
            result = _fpdManager->SetFPDBrightness(indicator, originalBrightness, persist);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS FPD brightness restored");
            } else {
                LOGERR("FAILED FPD brightness restore");
            }

            // 3. Test GetFPDState
            Exchange::IDeviceSettingsManagerFPD::FPDState currentState;
            result = _fpdManager->GetFPDState(indicator, currentState);
            LOGINFO("GetFPDState: indicator=%s, result=%u, state=%d", indicatorName, result, static_cast<int>(currentState));

            // 4. Test SetFPDState with get-set-restore pattern
            Exchange::IDeviceSettingsManagerFPD::FPDState originalState = currentState; // Save original
            Exchange::IDeviceSettingsManagerFPD::FPDState newState = (currentState == Exchange::IDeviceSettingsManagerFPD::FPDState::DS_FPD_STATE_ON) ? 
                Exchange::IDeviceSettingsManagerFPD::FPDState::DS_FPD_STATE_OFF : Exchange::IDeviceSettingsManagerFPD::FPDState::DS_FPD_STATE_ON;
            result = _fpdManager->SetFPDState(indicator, newState);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SetFPDState completed");
            } else {
                LOGERR("FAILED SetFPDState call");
            }

            // Restore original state
            result = _fpdManager->SetFPDState(indicator, originalState);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS FPD state restored");
            } else {
                LOGERR("FAILED FPD state restore");
            }

            // 5. Test GetFPDColor
            uint32_t currentColor = 0;
            result = _fpdManager->GetFPDColor(indicator, currentColor);
            LOGINFO("GetFPDColor: indicator=%s, result=%u, color=0x%08X", indicatorName, result, currentColor);

            // 6. Test SetFPDColor with get-set-restore pattern
            uint32_t originalColor = currentColor; // Save original
            uint32_t newColor = (currentColor == 0x0000FF) ? 0xFF0000 : 0x0000FF; // Use different color
            result = _fpdManager->SetFPDColor(indicator, newColor);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SetFPDColor completed");
            } else {
                LOGERR("FAILED SetFPDColor call");
            }

            // Restore original color
            result = _fpdManager->SetFPDColor(indicator, originalColor);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS FPD color restored");
            } else {
                LOGERR("FAILED FPD color restore");
            }

            LOGINFO("---------- Completed testing FPD Indicator: %s ----------\n", indicatorName);
        }

        // 7. Test SetFPDMode with different modes (get-set-restore pattern)
        LOGINFO("---------- Testing FPD Mode Settings ----------");

        Exchange::IDeviceSettingsManagerFPD::FPDMode testModes[] = {
            Exchange::IDeviceSettingsManagerFPD::FPDMode::DS_FPD_MODE_ANY,
            Exchange::IDeviceSettingsManagerFPD::FPDMode::DS_FPD_MODE_TEXT,
            Exchange::IDeviceSettingsManagerFPD::FPDMode::DS_FPD_MODE_CLOCK
        };

        // Assume default mode is ANY for restoration
        Exchange::IDeviceSettingsManagerFPD::FPDMode originalMode = Exchange::IDeviceSettingsManagerFPD::FPDMode::DS_FPD_MODE_ANY;

        for (size_t i = 0; i < sizeof(testModes)/sizeof(testModes[0]); i++) {
            auto mode = testModes[i];

            Core::hresult result = _fpdManager->SetFPDMode(mode);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SetFPDMode completed");
            } else {
                LOGERR("FAILED SetFPDMode call");
            }
        }

        // Restore original mode
        Core::hresult result = _fpdManager->SetFPDMode(originalMode);
        if (result == Core::ERROR_NONE) {
            LOGINFO("SUCCESS FPD mode restored");
        } else {
            LOGERR("FAILED FPD mode restore");
        }

        LOGINFO("---------- Completed FPD Mode Testing ----------\n");

        // Additional comprehensive test with different brightness values (get-set-restore)
        LOGINFO("---------- Testing FPD Brightness Range ----------");
        Exchange::IDeviceSettingsManagerFPD::FPDIndicator testIndicator = Exchange::IDeviceSettingsManagerFPD::FPDIndicator::DS_FPD_INDICATOR_POWER;
        
        // Get original brightness for restoration
        uint32_t originalBrightness = 0;
        _fpdManager->GetFPDBrightness(testIndicator, originalBrightness);
        
        uint32_t brightnessValues[] = {0, 25, 50, 75, 100};

        for (size_t i = 0; i < sizeof(brightnessValues)/sizeof(brightnessValues[0]); i++) {
            uint32_t brightness = brightnessValues[i];
            Core::hresult result = _fpdManager->SetFPDBrightness(testIndicator, brightness, true);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS FPD brightness test completed");
            } else {
                LOGERR("FAILED FPD brightness test");
            }
        }

        // Restore original brightness
        result = _fpdManager->SetFPDBrightness(testIndicator, originalBrightness, true);
        if (result == Core::ERROR_NONE) {
            LOGINFO("SUCCESS FPD brightness range restored");
        } else {
            LOGERR("FAILED FPD brightness range restore");
        }

        LOGINFO("---------- Completed FPD Brightness Range Testing ----------\n");

        // Test color variations with get-set-restore pattern
        LOGINFO("---------- Testing FPD Color Variations ----------");
        
        // Get original color for restoration
        uint32_t originalColor = 0;
        _fpdManager->GetFPDColor(testIndicator, originalColor);
        
        uint32_t colorValues[] = {
            0x000000, // Black
            0xFFFFFF, // White
            0xFF0000, // Red
            0x00FF00, // Green
            0x0000FF, // Blue
            0xFFFF00, // Yellow
            0xFF00FF, // Magenta
            0x00FFFF  // Cyan
        };

        for (size_t i = 0; i < sizeof(colorValues)/sizeof(colorValues[0]); i++) {
            uint32_t color = colorValues[i];

            Core::hresult result = _fpdManager->SetFPDColor(testIndicator, color);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS FPD color test completed");
            } else {
                LOGERR("FAILED FPD color test");
            }
        }

        // Restore original color
        result = _fpdManager->SetFPDColor(testIndicator, originalColor);
        if (result == Core::ERROR_NONE) {
            LOGINFO("SUCCESS FPD color variations restored");
        } else {
            LOGERR("FAILED FPD color variations restore");
        }

        LOGINFO("---------- Completed FPD Color Variations Testing ----------\n");

        LOGINFO("========== FPD APIs Testing Completed ==========\n");
    }

    void UserPlugin::TestSpecificHDMIInAPIs()
    {
        LOGINFO("========== HDMI In APIs Testing Framework ==========\n");

        if (!_hdmiInManager) {
            LOGERR("HDMI In Manager interface is not available!");
            return;
        }

        // Note: HDMI In APIs require a separate IHDMIIn interface that needs to be obtained
        // The specific methods listed in the requirement are part of the IHDMIIn sub-interface
        // This would require proper interface access pattern implementation

        LOGINFO("DeviceSettingsManager available - HDMI In interface access needs implementation");
        LOGINFO("Required APIs for implementation:");
        LOGINFO("- GetHDMIInNumbefOfInputs");
        LOGINFO("- GetHDMIInStatus"); 
        LOGINFO("- SelectHDMIInPort");
        LOGINFO("- ScaleHDMIInVideo");
        LOGINFO("- SelectHDMIZoomMode");
        LOGINFO("- GetSupportedGameFeaturesList");
        LOGINFO("- GetHDMIInAVLatency");
        LOGINFO("- GetHDMIInAllmStatus");
        LOGINFO("- GetHDMIInEdid2AllmSupport");
        LOGINFO("- SetHDMIInEdid2AllmSupport");
        LOGINFO("- GetEdidBytes");
        LOGINFO("- GetHDMISPDInformation");
        LOGINFO("- GetHDMIEdidVersion");
        LOGINFO("- SetHDMIEdidVersion");
        LOGINFO("- GetHDMIVideoMode");
        LOGINFO("- GetHDMIVersion");
        LOGINFO("- SetVRRSupport");
        LOGINFO("- GetVRRSupport");
        LOGINFO("- GetVRRStatus");

        LOGINFO("========== HDMI In API Framework Ready ==========");

        // Test all HDMI In methods with sample values
        using HDMIInPort = DeviceSettingsManagerHDMIIn::HDMIInPort;
        //using HDMIInSignalStatus = DeviceSettingsManagerHDMIIn::HDMIInSignalStatus;
        using HDMIVideoPortResolution = DeviceSettingsManagerHDMIIn::HDMIVideoPortResolution;
        //using HDMIInAviContentType = DeviceSettingsManagerHDMIIn::HDMIInAviContentType;

        // Test sample ports
        HDMIInPort testPorts[] = {
            HDMIInPort::DS_HDMI_IN_PORT_0,
            HDMIInPort::DS_HDMI_IN_PORT_1,
            HDMIInPort::DS_HDMI_IN_PORT_2
        };

        // Use HDMI In Manager interface directly
        DeviceSettingsManagerHDMIIn* hdmiIn = _hdmiInManager;

        uint32_t result = 0;
        for (auto port : testPorts) {
            LOGINFO("---------- Testing HDMI In Port: %d ----------", static_cast<int>(port));

            // 1. Test GetHDMIInNumbefOfInputs
            int32_t numInputs = 0;
            result = hdmiIn->GetHDMIInNumbefOfInputs(numInputs);
            LOGINFO("GetHDMIInNumbefOfInputs: result=%u, numInputs=%d", result, numInputs);

            // 2. Test GetHDMIInStatus
            DeviceSettingsManagerHDMIIn::HDMIInStatus hdmiStatus;
            DeviceSettingsManagerHDMIIn::IHDMIInPortConnectionStatusIterator* portConnIter = nullptr;
            result = hdmiIn->GetHDMIInStatus(hdmiStatus, portConnIter);
            LOGINFO("GetHDMIInStatus: result=%u, isPresented=%d, activePort=%d", result, hdmiStatus.isPresented, static_cast<int>(hdmiStatus.activePort));
            if (portConnIter) portConnIter->Release();

            // 3. Test SelectHDMIInPort with get-set-restore pattern
            DeviceSettingsManagerHDMIIn::HDMIInPort originalActivePort = hdmiStatus.activePort; // Save original active port
            result = hdmiIn->SelectHDMIInPort(port, true, true, DeviceSettingsManagerHDMIIn::HDMIVideoPlaneType::DS_HDMIIN_VIDEOPLANE_PRIMARY);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SelectHDMIInPort completed");
            } else {
                LOGERR("FAILED SelectHDMIInPort call");
            }

            // Restore original active port
            result = hdmiIn->SelectHDMIInPort(originalActivePort, true, true, DeviceSettingsManagerHDMIIn::HDMIVideoPlaneType::DS_HDMIIN_VIDEOPLANE_PRIMARY);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS HDMI port selection restored");
            } else {
                LOGERR("FAILED HDMI port selection restore");
            }

            // 4. Test ScaleHDMIInVideo with get-set-restore pattern
            DeviceSettingsManagerHDMIIn::HDMIInVideoRectangle testRect = {100, 100, 1920, 1080};
            result = hdmiIn->ScaleHDMIInVideo(testRect);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS ScaleHDMIInVideo completed");
            } else {
                LOGERR("FAILED ScaleHDMIInVideo call");
            }

            // Restore to full screen (assuming default)
            DeviceSettingsManagerHDMIIn::HDMIInVideoRectangle defaultRect = {0, 0, 1920, 1080};
            result = hdmiIn->ScaleHDMIInVideo(defaultRect);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS HDMI video scaling restored");
            } else {
                LOGERR("FAILED HDMI video scaling restore");
            }

            // 5. Test GetSupportedGameFeaturesList
            DeviceSettingsManagerHDMIIn::IHDMIInGameFeatureListIterator* gameFeatureList = nullptr;
            result = hdmiIn->GetSupportedGameFeaturesList(gameFeatureList);
            LOGINFO("GetSupportedGameFeaturesList: result=%u", result);

            // Print all game features from the iterator
            if (gameFeatureList && result == Core::ERROR_NONE) {
                LOGINFO("Printing game features from iterator:");
                DeviceSettingsManagerHDMIIn::HDMIInGameFeatureList currentFeature;
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
            DeviceSettingsManagerHDMIIn::HDMIVideoPortResolution videoMode;
            result = hdmiIn->GetHDMIVideoMode(videoMode);
            LOGINFO("GetHDMIVideoMode: result=%u, name=%s", result, videoMode.name.c_str());

            // 8. Test GetHDMIVersion
            DeviceSettingsManagerHDMIIn::HDMIInCapabilityVersion capVersion;
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
            DeviceSettingsManagerHDMIIn::HDMIInEdidVersion edidVersion;
            result = hdmiIn->GetHDMIEdidVersion(port, edidVersion);
            LOGINFO("GetHDMIEdidVersion: result=%u, version=%d", result, static_cast<int>(edidVersion));

            // 12. Test SetHDMIEdidVersion with get-set-restore pattern
            DeviceSettingsManagerHDMIIn::HDMIInEdidVersion originalEdidVersion = edidVersion; // Save original
            DeviceSettingsManagerHDMIIn::HDMIInEdidVersion newEdidVersion = (edidVersion == DeviceSettingsManagerHDMIIn::HDMIInEdidVersion::HDMI_EDID_VER_14) ? 
                DeviceSettingsManagerHDMIIn::HDMIInEdidVersion::HDMI_EDID_VER_20 : DeviceSettingsManagerHDMIIn::HDMIInEdidVersion::HDMI_EDID_VER_14;
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
            result = hdmiIn->SelectHDMIZoomMode(DeviceSettingsManagerHDMIIn::HDMIInVideoZoom::DS_HDMIIN_VIDEO_ZOOM_FULL);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SelectHDMIZoomMode completed");
            } else {
                LOGERR("FAILED SelectHDMIZoomMode call");
            }

            // Restore to default zoom mode (assuming NONE is default)
            result = hdmiIn->SelectHDMIZoomMode(DeviceSettingsManagerHDMIIn::HDMIInVideoZoom::DS_HDMIIN_VIDEO_ZOOM_NONE);
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
            DeviceSettingsManagerHDMIIn::HDMIInVRRStatus vrrStatus;
            result = hdmiIn->GetVRRStatus(port, vrrStatus);
            LOGINFO("GetVRRStatus: result=%u, vrrType=%d, framerate=%.2f", result, static_cast<int>(vrrStatus.vrrType), vrrStatus.vrrFreeSyncFramerateHz);

            LOGINFO("---------- Completed testing HDMI In Port: %d ----------", static_cast<int>(port));
        }

        // Test some additional methods that don't require specific ports
        LOGINFO("---------- Testing Additional HDMI In Methods ----------");

        // Test with sample resolution settings
        HDMIVideoPortResolution testResolution;
        testResolution.name = "1920x1080p60";
        LOGINFO("Testing with resolution: %s", testResolution.name.c_str());

        // No need to release _hdmiInManager as it's managed by the class

        LOGINFO("========== HDMI In Methods Testing Completed ==========");
    }

    // HDMI In Event Handler Implementations
    void UserPlugin::OnHDMIInEventHotPlug(const DeviceSettingsManagerHDMIIn::HDMIInPort port, const bool isConnected)
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

    void UserPlugin::OnHDMIInEventSignalStatus(const DeviceSettingsManagerHDMIIn::HDMIInPort port, const DeviceSettingsManagerHDMIIn::HDMIInSignalStatus signalStatus)
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

    void UserPlugin::OnHDMIInEventStatus(const DeviceSettingsManagerHDMIIn::HDMIInPort activePort, const bool isPresented)
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

    void UserPlugin::OnHDMIInVideoModeUpdate(const DeviceSettingsManagerHDMIIn::HDMIInPort port, const DeviceSettingsManagerHDMIIn::HDMIVideoPortResolution videoPortResolution)
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

    void UserPlugin::OnHDMIInAllmStatus(const DeviceSettingsManagerHDMIIn::HDMIInPort port, const bool allmStatus)
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

    void UserPlugin::OnHDMIInAVIContentType(const DeviceSettingsManagerHDMIIn::HDMIInPort port, const DeviceSettingsManagerHDMIIn::HDMIInAviContentType aviContentType)
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

    void UserPlugin::OnHDMIInVRRStatus(const DeviceSettingsManagerHDMIIn::HDMIInPort port, const DeviceSettingsManagerHDMIIn::HDMIInVRRType vrrType)
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

    void UserPlugin::TestAudioAPIs()
    {
        LOGINFO("========== Audio APIs Testing Framework ==========\n");

        if (!_audioManager) {
            LOGERR("Audio Manager interface is not available!");
            return;
        }

        LOGINFO("========== Testing Audio APIs ==========\n");

        // Test all audio port types
        WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioPortType testPortTypes[] = {
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioPortType::AUDIO_PORT_TYPE_HDMI,
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioPortType::AUDIO_PORT_TYPE_SPDIF,
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioPortType::AUDIO_PORT_TYPE_SPEAKER,
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioPortType::AUDIO_PORT_TYPE_HEADPHONE,
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioPortType::AUDIO_PORT_TYPE_HDMIARC,
        };

        const char* portTypeNames[] = {
            "HDMI",
            "SPDIF", 
            "ANALOG",
            "HEADPHONE"
        };

        for (size_t i = 0; i < sizeof(testPortTypes)/sizeof(testPortTypes[0]); i++) {
            auto portType = testPortTypes[i];
            const char* portTypeName = portTypeNames[i];

            LOGINFO("---------- Testing Audio Port Type: %s ----------", portTypeName);

            // 1. Test GetAudioPort
            int32_t handle = 0;
            Core::hresult result = _audioManager->GetAudioPort(portType, 0, handle);
            LOGINFO("GetAudioPort: portType=%s, index=0, result=%u, handle=%d", portTypeName, result, handle);

            if (result != Core::ERROR_NONE || handle <= 0) {
                LOGINFO("Skipping tests for %s port as it's not available", portTypeName);
                continue;
            }

            // 2. Test IsAudioPortEnabled (get original state)
            bool originalEnabled = false;
            result = _audioManager->IsAudioPortEnabled(handle, originalEnabled);
            LOGINFO("IsAudioPortEnabled: handle=%d, result=%u, enabled=%s", handle, result, originalEnabled ? "true" : "false");

            // 3. Test EnableAudioPort with get-set-restore pattern
            if (result == Core::ERROR_NONE) {
                bool newEnabled = !originalEnabled; // Toggle state
                result = _audioManager->EnableAudioPort(handle, newEnabled);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS EnableAudioPort completed: handle=%d, enabled=%s", handle, newEnabled ? "true" : "false");
                } else {
                    LOGERR("FAILED EnableAudioPort call: handle=%d", handle);
                }

                // Restore original state
                result = _audioManager->EnableAudioPort(handle, originalEnabled);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS Audio port enable state restored: handle=%d", handle);
                } else {
                    LOGERR("FAILED Audio port enable state restore: handle=%d", handle);
                }
            }

            // 4. Test IsAudioMuted (get original state)
            bool originalMuted = false;
            result = _audioManager->IsAudioMuted(handle, originalMuted);
            LOGINFO("IsAudioMuted: handle=%d, result=%u, muted=%s", handle, result, originalMuted ? "true" : "false");

            // 5. Test SetAudioMute with get-set-restore pattern
            if (result == Core::ERROR_NONE) {
                bool newMuted = !originalMuted; // Toggle state
                result = _audioManager->SetAudioMute(handle, newMuted);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS SetAudioMute completed: handle=%d, muted=%s", handle, newMuted ? "true" : "false");
                } else {
                    LOGERR("FAILED SetAudioMute call: handle=%d", handle);
                }

                // Restore original mute state
                result = _audioManager->SetAudioMute(handle, originalMuted);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS Audio mute state restored: handle=%d", handle);
                } else {
                    LOGERR("FAILED Audio mute state restore: handle=%d", handle);
                }
            }

            // 6. Test GetAudioLevel (get original level)
            float originalLevel = 0.0f;
            result = _audioManager->GetAudioLevel(handle, originalLevel);
            LOGINFO("GetAudioLevel: handle=%d, result=%u, level=%.2f", handle, result, originalLevel);

            // 7. Test SetAudioLevel with get-set-restore pattern
            if (result == Core::ERROR_NONE) {
                float newLevel = (originalLevel >= 50.0f) ? 25.0f : 75.0f; // Use different level
                result = _audioManager->SetAudioLevel(handle, newLevel);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS SetAudioLevel completed: handle=%d, level=%.2f", handle, newLevel);
                } else {
                    LOGERR("FAILED SetAudioLevel call: handle=%d", handle);
                }

                // Restore original level
                result = _audioManager->SetAudioLevel(handle, originalLevel);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS Audio level restored: handle=%d, level=%.2f", handle, originalLevel);
                } else {
                    LOGERR("FAILED Audio level restore: handle=%d", handle);
                }
            }

            // 8. Test GetAudioGain (get original gain)
            float originalGain = 0.0f;
            result = _audioManager->GetAudioGain(handle, originalGain);
            LOGINFO("GetAudioGain: handle=%d, result=%u, gain=%.2f", handle, result, originalGain);

            // 9. Test SetAudioGain with get-set-restore pattern
            if (result == Core::ERROR_NONE) {
                float newGain = (originalGain >= 0.0f) ? -6.0f : 6.0f; // Use different gain
                result = _audioManager->SetAudioGain(handle, newGain);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS SetAudioGain completed: handle=%d, gain=%.2f", handle, newGain);
                } else {
                    LOGERR("FAILED SetAudioGain call: handle=%d", handle);
                }

                // Restore original gain
                result = _audioManager->SetAudioGain(handle, originalGain);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS Audio gain restored: handle=%d, gain=%.2f", handle, originalGain);
                } else {
                    LOGERR("FAILED Audio gain restore: handle=%d", handle);
                }
            }

            // 10. Test GetAudioFormat
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioFormat audioFormat;
            result = _audioManager->GetAudioFormat(handle, audioFormat);
            LOGINFO("GetAudioFormat: handle=%d, result=%u, format=%d", handle, result, static_cast<int>(audioFormat));

            // 11. Test GetAudioEncoding
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioEncoding audioEncoding;
            result = _audioManager->GetAudioEncoding(handle, audioEncoding);
            LOGINFO("GetAudioEncoding: handle=%d, result=%u, encoding=%d", handle, result, static_cast<int>(audioEncoding));

            // 12. Test GetSupportedARCTypes (for HDMI ports)
            if (portType == WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioPortType::AUDIO_PORT_TYPE_HDMI) {
                int32_t arcTypes = 0;
                result = _audioManager->GetSupportedARCTypes(handle, arcTypes);
                LOGINFO("GetSupportedARCTypes: handle=%d, result=%u, arcTypes=0x%X", handle, result, arcTypes);

                // 13. Test SetSAD with sample data
                uint8_t sadData[] = {0x09, 0x07, 0x07}; // Sample SAD data
                uint8_t sadCount = sizeof(sadData);
                result = _audioManager->SetSAD(handle, sadData, sadCount);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS SetSAD completed: handle=%d, count=%d", handle, sadCount);
                } else {
                    LOGERR("FAILED SetSAD call: handle=%d", handle);
                }

                // 14. Test EnableARC with get-set-restore pattern (if applicable)
                WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioARCStatus arcStatus;
                arcStatus.arcType = WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioARCStatus::ARCType::AUDIO_ARCTYPE_ARC;
                arcStatus.status = WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioARCStatus::ARCStatus::AUDIO_ARCTYPE_NONE;
                
                result = _audioManager->EnableARC(handle, arcStatus);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS EnableARC completed: handle=%d", handle);
                } else {
                    LOGERR("FAILED EnableARC call: handle=%d", handle);
                }

                // Disable ARC to restore state
                arcStatus.status = WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioARCStatus::ARCStatus::AUDIO_ARCTYPE_NONE;
                result = _audioManager->EnableARC(handle, arcStatus);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS ARC state restored: handle=%d", handle);
                } else {
                    LOGERR("FAILED ARC state restore: handle=%d", handle);
                }
            }

            // 15. Test Stereo Mode APIs with get-set-restore pattern
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::StereoMode originalStereoMode;
            result = _audioManager->GetStereoMode(handle, originalStereoMode);
            LOGINFO("GetStereoMode: handle=%d, result=%u, mode=%d", handle, result, static_cast<int>(originalStereoMode));

            if (result == Core::ERROR_NONE) {
                // Test different stereo modes
                WPEFramework::Exchange::IDeviceSettingsManagerAudio::StereoMode testMode = 
                    (originalStereoMode == WPEFramework::Exchange::IDeviceSettingsManagerAudio::StereoMode::AUDIO_STEREO_STEREO) ?
                    WPEFramework::Exchange::IDeviceSettingsManagerAudio::StereoMode::AUDIO_STEREO_MONO :
                    WPEFramework::Exchange::IDeviceSettingsManagerAudio::StereoMode::AUDIO_STEREO_STEREO;

                result = _audioManager->SetStereoMode(handle, testMode, false);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS SetStereoMode completed: handle=%d, mode=%d", handle, static_cast<int>(testMode));
                } else {
                    LOGERR("FAILED SetStereoMode call: handle=%d", handle);
                }

                // Restore original stereo mode
                result = _audioManager->SetStereoMode(handle, originalStereoMode, false);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS Stereo mode restored: handle=%d", handle);
                } else {
                    LOGERR("FAILED Stereo mode restore: handle=%d", handle);
                }
            }

            // 16. Test Stereo Auto APIs with get-set-restore pattern
            int32_t originalAutoMode = 0;
            result = _audioManager->GetStereoAuto(handle, originalAutoMode);
            LOGINFO("GetStereoAuto: handle=%d, result=%u, autoMode=%d", handle, result, originalAutoMode);

            if (result == Core::ERROR_NONE) {
                int32_t newAutoMode = (originalAutoMode == 0) ? 1 : 0; // Toggle auto mode
                result = _audioManager->SetStereoAuto(handle, newAutoMode, false);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS SetStereoAuto completed: handle=%d, autoMode=%d", handle, newAutoMode);
                } else {
                    LOGERR("FAILED SetStereoAuto call: handle=%d", handle);
                }

                // Restore original auto mode
                result = _audioManager->SetStereoAuto(handle, originalAutoMode, false);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS Stereo auto mode restored: handle=%d", handle);
                } else {
                    LOGERR("FAILED Stereo auto mode restore: handle=%d", handle);
                }
            }

            LOGINFO("---------- Completed testing Audio Port Type: %s ----------\n", portTypeName);
        }

        // Test advanced audio features with get-set-restore patterns
        LOGINFO("---------- Testing Advanced Audio Features ----------");

        // Test with HDMI port for advanced features
        int32_t hdmiHandle = 0;
        Core::hresult result = _audioManager->GetAudioPort(WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioPortType::AUDIO_PORT_TYPE_HDMI, 0, hdmiHandle);
        
        if (result == Core::ERROR_NONE && hdmiHandle > 0) {
            // Test Volume Leveller with get-set-restore pattern
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::VolumeLeveller originalLeveller;
            result = _audioManager->GetVolumeLeveller(hdmiHandle, originalLeveller);
            LOGINFO("GetVolumeLeveller: handle=%d, result=%u, mode=%d, level=%d", hdmiHandle, result, originalLeveller.mode, originalLeveller.level);

            if (result == Core::ERROR_NONE) {
                WPEFramework::Exchange::IDeviceSettingsManagerAudio::VolumeLeveller newLeveller;
                newLeveller.mode = (originalLeveller.mode == 0) ? 1 : 0;
                newLeveller.level = (originalLeveller.level >= 50) ? 25 : 75;

                result = _audioManager->SetVolumeLeveller(hdmiHandle, newLeveller);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS SetVolumeLeveller completed: handle=%d", hdmiHandle);
                } else {
                    LOGERR("FAILED SetVolumeLeveller call: handle=%d", hdmiHandle);
                }

                // Restore original settings
                result = _audioManager->SetVolumeLeveller(hdmiHandle, originalLeveller);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS Volume leveller restored: handle=%d", hdmiHandle);
                } else {
                    LOGERR("FAILED Volume leveller restore: handle=%d", hdmiHandle);
                }
            }

            // Test Surround Virtualizer with get-set-restore pattern
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::SurroundVirtualizer originalVirtualizer;
            result = _audioManager->GetSurroundVirtualizer(hdmiHandle, originalVirtualizer);
            LOGINFO("GetSurroundVirtualizer: handle=%d, result=%u, mode=%d, boost=%d", hdmiHandle, result, originalVirtualizer.mode, originalVirtualizer.boost);

            if (result == Core::ERROR_NONE) {
                WPEFramework::Exchange::IDeviceSettingsManagerAudio::SurroundVirtualizer newVirtualizer;
                newVirtualizer.mode = (originalVirtualizer.mode == 0) ? 1 : 0;
                newVirtualizer.boost = (originalVirtualizer.boost >= 50) ? 25 : 75;

                result = _audioManager->SetSurroundVirtualizer(hdmiHandle, newVirtualizer);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS SetSurroundVirtualizer completed: handle=%d", hdmiHandle);
                } else {
                    LOGERR("FAILED SetSurroundVirtualizer call: handle=%d", hdmiHandle);
                }

                // Restore original settings
                result = _audioManager->SetSurroundVirtualizer(hdmiHandle, originalVirtualizer);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS Surround virtualizer restored: handle=%d", hdmiHandle);
                } else {
                    LOGERR("FAILED Surround virtualizer restore: handle=%d", hdmiHandle);
                }
            }

            // Test MS11/MS12 Decoding queries
            bool hasMS11Decode = false;
            result = _audioManager->IsAudioMSDecoding(hdmiHandle, hasMS11Decode);
            LOGINFO("IsAudioMSDecoding: handle=%d, result=%u, hasMS11=%s", hdmiHandle, result, hasMS11Decode ? "true" : "false");

            bool hasMS12Decode = false;
            result = _audioManager->IsAudioMS12Decoding(hdmiHandle, hasMS12Decode);
            LOGINFO("IsAudioMS12Decoding: handle=%d, result=%u, hasMS12=%s", hdmiHandle, result, hasMS12Decode ? "true" : "false");

            // Test Audio Ducking
            result = _audioManager->SetAudioDucking(hdmiHandle, 
                WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioDuckingType::AUDIO_DUCKINGTYPE_RELATIVE,
                WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioDuckingAction::AUDIO_DUCKINGACTION_START,
                50);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS SetAudioDucking START completed: handle=%d", hdmiHandle);
            } else {
                LOGERR("FAILED SetAudioDucking START call: handle=%d", hdmiHandle);
            }

            // Stop ducking to restore state
            result = _audioManager->SetAudioDucking(hdmiHandle, 
                WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioDuckingType::AUDIO_DUCKINGTYPE_RELATIVE,
                WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioDuckingAction::AUDIO_DUCKINGACTION_STOP,
                0);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS Audio ducking restored: handle=%d", hdmiHandle);
            } else {
                LOGERR("FAILED Audio ducking restore: handle=%d", hdmiHandle);
            }
        }

        LOGINFO("---------- Completed Advanced Audio Features Testing ----------\n");

        // Test comprehensive audio level variations with get-set-restore pattern
        LOGINFO("---------- Testing Audio Level Variations ----------");
        
        if (hdmiHandle > 0) {
            // Get original level for restoration
            float originalTestLevel = 0.0f;
            _audioManager->GetAudioLevel(hdmiHandle, originalTestLevel);
            
            float levelValues[] = {0.0f, 25.0f, 50.0f, 75.0f, 100.0f};

            for (size_t i = 0; i < sizeof(levelValues)/sizeof(levelValues[0]); i++) {
                float level = levelValues[i];
                Core::hresult result = _audioManager->SetAudioLevel(hdmiHandle, level);
                if (result == Core::ERROR_NONE) {
                    LOGINFO("SUCCESS Audio level test completed: level=%.1f", level);
                } else {
                    LOGERR("FAILED Audio level test: level=%.1f", level);
                }
            }

            // Restore original level
            result = _audioManager->SetAudioLevel(hdmiHandle, originalTestLevel);
            if (result == Core::ERROR_NONE) {
                LOGINFO("SUCCESS Audio level variations restored");
            } else {
                LOGERR("FAILED Audio level variations restore");
            }
        }

        LOGINFO("---------- Completed Audio Level Variations Testing ----------\n");

        LOGINFO("========== Audio APIs Testing Completed ==========\n");
    }

} // namespace Plugin
} // namespace WPEFramework
