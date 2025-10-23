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

    UserPlugin::UserPlugin() : _service(nullptr), _connectionId(0), _fpdManager(nullptr), _hdmiInManager(nullptr), _pwrMgrNotification(*this), _hdmiInNotification(*this)
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

        TestFPDAPIs();
        // Test HDMI In methods with sample values
        TestSpecificHDMIInAPIs();

        Exchange::JUserPlugin::Register(*this, this);

        /*try
        {
            //TODO(MROLLINS) this is probably per process so we either need to be running in our own process or be carefull no other plugin is calling it
            device::Manager::Initialize();
            LOGINFO("device::Manager::Initialize success");
        }
        catch(...)
        {
            LOGINFO("device::Manager::Initialize failed");
        }
 
        string portName = "HDMI0"; // Replace with the actual port name as needed

        device::AudioOutputPort aPort = device::Host::getInstance().getAudioOutputPort(portName);
    
    	if (aPort.getName().empty()) {
            LOGERR("AudioOutputPort '%s' is not valid!", portName.c_str());
        } else {
            float volumeLevel;
            volumeLevel = aPort.getLevel();
//            GetVolumeLevel(portName, volumeLevel);
            LOGINFO("Volume level for port %s: Vol Level[HDMI0] %f", portName.c_str(), volumeLevel);
        }*/

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

            // 2. Test SetFPDBrightness
            uint32_t newBrightness = 75;
            bool persist = true;
            result = _fpdManager->SetFPDBrightness(indicator, newBrightness, persist);
            LOGINFO("SetFPDBrightness: indicator=%s, result=%u, brightness=%u, persist=%s", indicatorName, result, newBrightness, persist ? "true" : "false");

            // Verify the brightness was set
            uint32_t verifyBrightness = 0;
            result = _fpdManager->GetFPDBrightness(indicator, verifyBrightness);
            LOGINFO("GetFPDBrightness (verify): indicator=%s, result=%u, brightness=%u", indicatorName, result, verifyBrightness);

            // 3. Test GetFPDState
            Exchange::IDeviceSettingsManagerFPD::FPDState currentState;
            result = _fpdManager->GetFPDState(indicator, currentState);
            LOGINFO("GetFPDState: indicator=%s, result=%u, state=%d", indicatorName, result, static_cast<int>(currentState));

            // 4. Test SetFPDState
            Exchange::IDeviceSettingsManagerFPD::FPDState newState = Exchange::IDeviceSettingsManagerFPD::FPDState::DS_FPD_STATE_ON;
            result = _fpdManager->SetFPDState(indicator, newState);
            LOGINFO("SetFPDState: indicator=%s, result=%u, state=%d (ON)", indicatorName, result, static_cast<int>(newState));

            // Verify the state was set
            Exchange::IDeviceSettingsManagerFPD::FPDState verifyState;
            result = _fpdManager->GetFPDState(indicator, verifyState);
            LOGINFO("GetFPDState (verify): indicator=%s, result=%u, state=%d", indicatorName, result, static_cast<int>(verifyState));

            // 5. Test GetFPDColor
            uint32_t currentColor = 0;
            result = _fpdManager->GetFPDColor(indicator, currentColor);
            LOGINFO("GetFPDColor: indicator=%s, result=%u, color=0x%08X", indicatorName, result, currentColor);

            // 6. Test SetFPDColor
            uint32_t newColor = 0x0000FF; // Blue color
            result = _fpdManager->SetFPDColor(indicator, newColor);
            LOGINFO("SetFPDColor: indicator=%s, result=%u, color=0x%08X (Blue)", indicatorName, result, newColor);

            // Verify the color was set
            uint32_t verifyColor = 0;
            result = _fpdManager->GetFPDColor(indicator, verifyColor);
            LOGINFO("GetFPDColor (verify): indicator=%s, result=%u, color=0x%08X", indicatorName, result, verifyColor);

            // Test with different state - OFF
            newState = Exchange::IDeviceSettingsManagerFPD::FPDState::DS_FPD_STATE_OFF;
            result = _fpdManager->SetFPDState(indicator, newState);
            LOGINFO("SetFPDState: indicator=%s, result=%u, state=%d (OFF)", indicatorName, result, static_cast<int>(newState));

            LOGINFO("---------- Completed testing FPD Indicator: %s ----------\n", indicatorName);
        }

        // 7. Test SetFPDMode with different modes
        LOGINFO("---------- Testing FPD Mode Settings ----------");
        
        Exchange::IDeviceSettingsManagerFPD::FPDMode testModes[] = {
            Exchange::IDeviceSettingsManagerFPD::FPDMode::DS_FPD_MODE_ANY,
            Exchange::IDeviceSettingsManagerFPD::FPDMode::DS_FPD_MODE_TEXT,
            Exchange::IDeviceSettingsManagerFPD::FPDMode::DS_FPD_MODE_CLOCK
        };

        const char* modeNames[] = {
            "ANY",
            "TEXT",
            "CLOCK"
        };

        for (size_t i = 0; i < sizeof(testModes)/sizeof(testModes[0]); i++) {
            auto mode = testModes[i];
            const char* modeName = modeNames[i];
            
            Core::hresult result = _fpdManager->SetFPDMode(mode);
            LOGINFO("SetFPDMode: mode=%s, result=%u", modeName, result);
        }

        LOGINFO("---------- Completed FPD Mode Testing ----------\n");

        // Additional comprehensive test with different brightness values
        LOGINFO("---------- Testing FPD Brightness Range ----------");
        auto testIndicator = Exchange::IDeviceSettingsManagerFPD::FPDIndicator::DS_FPD_INDICATOR_POWER;
        uint32_t brightnessValues[] = {0, 25, 50, 75, 100};
        
        for (size_t i = 0; i < sizeof(brightnessValues)/sizeof(brightnessValues[0]); i++) {
            uint32_t brightness = brightnessValues[i];
            Core::hresult result = _fpdManager->SetFPDBrightness(testIndicator, brightness, true);
            LOGINFO("SetFPDBrightness: brightness=%u, result=%u", brightness, result);
            
            uint32_t verifyBrightness = 0;
            result = _fpdManager->GetFPDBrightness(testIndicator, verifyBrightness);
            LOGINFO("GetFPDBrightness (verify): brightness=%u, result=%u", verifyBrightness, result);
        }

        LOGINFO("---------- Completed FPD Brightness Range Testing ----------\n");

        // Test color variations
        LOGINFO("---------- Testing FPD Color Variations ----------");
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
        
        const char* colorNames[] = {
            "Black",
            "White", 
            "Red",
            "Green",
            "Blue",
            "Yellow",
            "Magenta",
            "Cyan"
        };

        for (size_t i = 0; i < sizeof(colorValues)/sizeof(colorValues[0]); i++) {
            uint32_t color = colorValues[i];
            const char* colorName = colorNames[i];
            
            Core::hresult result = _fpdManager->SetFPDColor(testIndicator, color);
            LOGINFO("SetFPDColor: color=%s (0x%08X), result=%u", colorName, color, result);
            
            uint32_t verifyColor = 0;
            result = _fpdManager->GetFPDColor(testIndicator, verifyColor);
            LOGINFO("GetFPDColor (verify): color=0x%08X, result=%u", verifyColor, result);
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

            // 3. Test SelectHDMIInPort
            result = hdmiIn->SelectHDMIInPort(port, true, true, DeviceSettingsManagerHDMIIn::DS_HDMIIN_VIDEOPLANE_PRIMARY);
            LOGINFO("SelectHDMIInPort: result=%u, port=%d", result, static_cast<int>(port));

            // 4. Test ScaleHDMIInVideo
            DeviceSettingsManagerHDMIIn::HDMIInVideoRectangle rect = {100, 100, 1920, 1080};
            result = hdmiIn->ScaleHDMIInVideo(rect);
            LOGINFO("ScaleHDMIInVideo: result=%u, x=%d, y=%d, w=%d, h=%d", result, rect.x, rect.y, rect.width, rect.height);

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

            // 12. Test SetHDMIEdidVersion
            result = hdmiIn->SetHDMIEdidVersion(port, edidVersion);
            LOGINFO("SetHDMIEdidVersion: result=%u", result);

            // 13. Test GetHDMIInAllmStatus
            bool allmStatus = false;
            result = hdmiIn->GetHDMIInAllmStatus(port, allmStatus);
            LOGINFO("GetHDMIInAllmStatus: result=%u, allmStatus=%d", result, allmStatus);

            // 14. Test GetHDMIInEdid2AllmSupport
            bool allmSupport = false;
            result = hdmiIn->GetHDMIInEdid2AllmSupport(port, allmSupport);
            LOGINFO("GetHDMIInEdid2AllmSupport: result=%u, allmSupport=%d", result, allmSupport);

            // 15. Test SetHDMIInEdid2AllmSupport
            result = hdmiIn->SetHDMIInEdid2AllmSupport(port, allmSupport);
            LOGINFO("SetHDMIInEdid2AllmSupport: result=%u", result);

            // 16. Test SelectHDMIZoomMode
            result = hdmiIn->SelectHDMIZoomMode(DeviceSettingsManagerHDMIIn::DS_HDMIIN_VIDEO_ZOOM_FULL);
            LOGINFO("SelectHDMIZoomMode: result=%u", result);

            // 17. Test GetVRRSupport
            bool vrrSupport = false;
            result = hdmiIn->GetVRRSupport(port, vrrSupport);
            LOGINFO("GetVRRSupport: result=%u, vrrSupport=%d", result, vrrSupport);

            // 18. Test SetVRRSupport
            result = hdmiIn->SetVRRSupport(port, vrrSupport);
            LOGINFO("SetVRRSupport: result=%u", result);

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

} // namespace Plugin
} // namespace WPEFramework
