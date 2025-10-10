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

// Use proper FPD types from IDeviceSettingsManager interface
using FPDIndicator = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDIndicator;
using FPDState = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDState;

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

    UserPlugin::UserPlugin() : _service(nullptr), _connectionId(0), _deviceSettingsManager(nullptr), _pwrMgrNotification(*this)
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

    // Removed: _fpdManager assignment, use _deviceSettingsManager for FPD APIs

        _deviceSettingsManager = _service->QueryInterfaceByCallsign<Exchange::IDeviceSettingsManager>("org.rdk.DeviceSettingsManager");
        ASSERT(_deviceSettingsManager != nullptr);

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
        // Removed: _fpdManager->Release(); // No longer needed
        if (_deviceSettingsManager) {
            _deviceSettingsManager->Release();
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

        // Get FPD interface from DeviceSettingsManager
        Exchange::IDeviceSettingsManager::IFPD* fpd = nullptr;
        if (_deviceSettingsManager) {
            fpd = _deviceSettingsManager->QueryInterface<Exchange::IDeviceSettingsManager::IFPD>();
            if (fpd == nullptr) {
                LOGINFO("Failed to get FPD interface");
            }
        }
        
        if (fpd) {
            fpd->GetFPDBrightness(FPDIndicator::DS_FPD_INDICATOR_POWER, brightness);
            LOGINFO("GetFPDBrightness brightness: %d", brightness);
            brightness = 50;
            persist = 1;
            LOGINFO("SetFPDBrightness brightness: %d, persist: %d", brightness, persist);
            fpd->SetFPDBrightness(FPDIndicator::DS_FPD_INDICATOR_POWER, brightness, persist);
            fpd->GetFPDState(FPDIndicator::DS_FPD_INDICATOR_POWER, fpdState);
            LOGINFO("GetFPDState state: %d", fpdState);
            fpdState = FPDState::DS_FPD_STATE_ON;
            LOGINFO("SetFPDState state:%d", fpdState);
            fpd->SetFPDState(FPDIndicator::DS_FPD_INDICATOR_POWER, fpdState);
            fpd->Release();
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

    void UserPlugin::TestSpecificHDMIInAPIs()
    {
        LOGINFO("========== HDMI In APIs Testing Framework ==========");
        
        if (!_deviceSettingsManager) {
            LOGERR("DeviceSettingsManager interface is not available!");
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
        using HDMIInPort = Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort;
        //using HDMIInSignalStatus = Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInSignalStatus;
        using HDMIVideoPortResolution = Exchange::IDeviceSettingsManager::IHDMIIn::HDMIVideoPortResolution;
        //using HDMIInAviContentType = Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInAviContentType;

        // Test sample ports
        HDMIInPort testPorts[] = {
            HDMIInPort::DS_HDMI_IN_PORT_0,
            HDMIInPort::DS_HDMI_IN_PORT_1,
            HDMIInPort::DS_HDMI_IN_PORT_2
        };

        // Get HDMI In interface from DeviceSettingsManager
        Exchange::IDeviceSettingsManager::IHDMIIn* hdmiIn = nullptr;
        if (_deviceSettingsManager) {
            hdmiIn = _deviceSettingsManager->QueryInterface<Exchange::IDeviceSettingsManager::IHDMIIn>();
            if (hdmiIn == nullptr) {
                LOGINFO("Failed to get HDMI In interface");
                return;
            }
        }

        uint32_t result = 0;
        for (auto port : testPorts) {
            LOGINFO("---------- Testing HDMI In Port: %d ----------", static_cast<int>(port));

            // 1. Test GetHDMIInNumbefOfInputs
            int32_t numInputs = 0;
            result = hdmiIn->GetHDMIInNumbefOfInputs(numInputs);
            LOGINFO("GetHDMIInNumbefOfInputs: result=%u, numInputs=%d", result, numInputs);

            // 2. Test GetHDMIInStatus
            Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInStatus hdmiStatus;
            Exchange::IDeviceSettingsManager::IHDMIIn::IHDMIInPortConnectionStatusIterator* portConnIter = nullptr;
            result = hdmiIn->GetHDMIInStatus(hdmiStatus, portConnIter);
            LOGINFO("GetHDMIInStatus: result=%u, isPresented=%d, activePort=%d", result, hdmiStatus.isPresented, static_cast<int>(hdmiStatus.activePort));
            if (portConnIter) portConnIter->Release();

            // 3. Test SelectHDMIInPort
            result = hdmiIn->SelectHDMIInPort(port, true, true, Exchange::IDeviceSettingsManager::IHDMIIn::DS_HDMIIN_VIDEOPLANE_PRIMARY);
            LOGINFO("SelectHDMIInPort: result=%u, port=%d", result, static_cast<int>(port));

            // 4. Test ScaleHDMIInVideo
            Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInVideoRectangle rect = {100, 100, 1920, 1080};
            result = hdmiIn->ScaleHDMIInVideo(rect);
            LOGINFO("ScaleHDMIInVideo: result=%u, x=%d, y=%d, w=%d, h=%d", result, rect.x, rect.y, rect.width, rect.height);

            // 5. Test GetSupportedGameFeaturesList
            Exchange::IDeviceSettingsManager::IHDMIIn::IHDMIInGameFeatureListIterator* gameFeatureList = nullptr;
            result = hdmiIn->GetSupportedGameFeaturesList(gameFeatureList);
            LOGINFO("GetSupportedGameFeaturesList: result=%u", result);
            if (gameFeatureList) gameFeatureList->Release();

            // 6. Test GetHDMIInAVLatency
            uint32_t videoLatency = 0, audioLatency = 0;
            result = hdmiIn->GetHDMIInAVLatency(videoLatency, audioLatency);
            LOGINFO("GetHDMIInAVLatency: result=%u, videoLatency=%u, audioLatency=%u", result, videoLatency, audioLatency);

            // 7. Test GetHDMIVideoMode
            Exchange::IDeviceSettingsManager::IHDMIIn::HDMIVideoPortResolution videoMode;
            result = hdmiIn->GetHDMIVideoMode(videoMode);
            LOGINFO("GetHDMIVideoMode: result=%u, name=%s", result, videoMode.name.c_str());

            // 8. Test GetHDMIVersion
            Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInCapabilityVersion capVersion;
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
            Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInEdidVersion edidVersion;
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
            result = hdmiIn->SelectHDMIZoomMode(Exchange::IDeviceSettingsManager::IHDMIIn::DS_HDMIIN_VIDEO_ZOOM_FULL);
            LOGINFO("SelectHDMIZoomMode: result=%u", result);

            // 17. Test GetVRRSupport
            bool vrrSupport = false;
            result = hdmiIn->GetVRRSupport(port, vrrSupport);
            LOGINFO("GetVRRSupport: result=%u, vrrSupport=%d", result, vrrSupport);

            // 18. Test SetVRRSupport
            result = hdmiIn->SetVRRSupport(port, vrrSupport);
            LOGINFO("SetVRRSupport: result=%u", result);

            // 19. Test GetVRRStatus
            Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInVRRStatus vrrStatus;
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

        // Release the HDMI In interface
        if (hdmiIn) {
            hdmiIn->Release();
        }
        
        LOGINFO("========== HDMI In Methods Testing Completed ==========");
    }
} // namespace Plugin
} // namespace WPEFramework
