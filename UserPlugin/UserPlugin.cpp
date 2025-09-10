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
#include "UtilsIarm.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 1
#define API_VERSION_NUMBER_PATCH 0

using namespace std;
using namespace WPEFramework;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

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

    UserPlugin::UserPlugin() : _service(nullptr), _connectionId(0), _pwrMgrNotification(*this)
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

    
        if (_powerManager && &_pwrMgrNotification) 
        {
            _powerManager->Register(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
        } else {
            LOGERR("PowerManager or Notification is NULL");
        }

        Exchange::JUserPlugin::Register(*this, this);

        // On success return empty, to indicate there is no error text.
        if (Utils::IARM::init())
        {
            LOGINFO("Utils::IARM::initialize failed");
        }

        try
        {
            //TODO(MROLLINS) this is probably per process so we either need to be running in our own process or be carefull no other plugin is calling it
            device::Manager::Initialize();
            LOGINFO("device::Manager::Initialize success");
        }
        catch(...)
        {
            LOGINFO("device::Manager::Initialize failed");
        }

    //    GetCurrentResolutionValue();

        return (string());
    }

    /* virtual */ void UserPlugin::Deinitialize(PluginHost::IShell* service)
    {
        LOGINFO("Deinitialize");

        _powerManager->Release();
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

        LOGINFO("getVolumeLevel, port name: %s", port.c_str());
        device::AudioOutputPort aPort = device::Host::getInstance().getAudioOutputPort(port);
        if (aPort.getName().empty()){
            LOGERR("AudioOutputPort '%s' is not valid!", port.c_str());
            return Core::ERROR_GENERAL;
        }
            volumelevel = aPort.getLevel();
            level = to_string(volumelevel);
            LOGINFO("getVolumeLevel, volumeLevel: %s", level.c_str());
    //    volumelevel = aPort.getLevel();

        return Core::ERROR_NONE;
    }

    Core::hresult UserPlugin::GetCurrentResolutionValue(std::string& resolution) const
    {
        std::string strVideoPort = device::Host::getInstance().getDefaultVideoPortName();

        string videoDisplay = strVideoPort;

        try
        {
            int width = 0;
            int height = 0;
            bool progressive = false;

            device::VideoOutputPort &vPort = device::Host::getInstance().getVideoOutputPort(videoDisplay);

            string res = vPort.getResolution().getName();

            if(res.rfind("480", 0) == 0)
            {
                width =  720;
                height = 480;
            }
            else if(res.rfind("576", 0) == 0)
            {
                width =  720;
                height = 576;
            }
            else if(res.rfind("720", 0) == 0)
            {
                width =  1280;
                height = 720;
            }
            else if(res.rfind("768", 0) == 0)
            {
                width =  1366;
                height = 768;
            }
            else if(res.rfind("1080", 0) == 0)
            {
                width =  1920;
                height = 1080;
            }
            else if(res.rfind("2160", 0) == 0)
            {
                width =  3840;
                height = 2160;
            }
            else if(res.rfind("4096x2160", 0) == 0)
            {
                width =  4096;
                height = 2160;
            }
            else
            {
                width =  1280;
                height = 720;
            }
    
            if(res.find('p') != std::string::npos) {
                progressive = true;
            }

            LOGINFO("Current Res. for port %s: Resolution %s width %d height %d progressive %d", videoDisplay.c_str(), res.c_str(), width, height, progressive);

            resolution = std::to_string(width) + "X" + std::to_string(height);
        }
        catch(const device::Exception& err)
        {
            LOG_DEVICE_EXCEPTION1(videoDisplay);
        }
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
