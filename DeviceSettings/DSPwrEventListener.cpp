/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

#include "DSPwrEventListener.h"
#include "DSProductTraitsHandler.h"  
#include "DSController.h"
#include "UtilsLogging.h"
#include "DeviceSettingsTypes.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

//extern profile_t profileType;

#include "frontPanelIndicator.hpp"
//#include "videoOutputPort.hpp"
#include "host.hpp"
//#include "audioOutputPortType.hpp"
//#include "videoOutputPortType.hpp"
#include "exception.hpp"
#include "manager.hpp"
#include "UtilsLogging.h"

extern "C" {
    #include "dsRpc.h"
    //#include "dsAudioSettings.h"
    //#include "plat_power.h"
    //#include "rdkProfile.h"
}

#define PWRMGR_REBOOT_REASON_MAINTENANCE "MAINTENANCE_REBOOT"
//#define RDK_PROFILE "RDK_PROFILE"
//#define PROFILE_STR_TV "TV"
//#define PROFILE_STR_STB "STB"

extern IARM_Result_t _dsGetAudioPort(void *arg);
extern IARM_Result_t _dsEnableAudioPort(void *arg);
extern IARM_Result_t _dsSetFPState(void *arg);
extern IARM_Result_t _dsGetEnablePersist(void *arg);
extern void _setEASAudioMode();
extern IARM_Result_t _dsGetVideoPort(void *arg);
extern IARM_Result_t _dsEnableVideoPort(void *arg);

namespace WPEFramework {
namespace Plugin {

DSPwrEventListener* DSPwrEventListener::_instance = nullptr;
static DSProductTraits::UXController* ux = nullptr;

DSPwrEventListener::DSPwrEventListener()
    : _pwrEventHandlerThreadID(0)
    , _stopThread(false)
    , _registeredPowerEventHandler(false)
    , _curState(PowerState::POWER_STATE_STANDBY)
    , _pwrMgrNotification(*this)
    , _service(nullptr)
{
    LOGINFO("DSPwrEventListener Constructor");
    memset(_standbyVideoPortSetting, 0, sizeof(_standbyVideoPortSetting));
    DSPwrEventListener::_instance = this;
}

DSPwrEventListener::~DSPwrEventListener()
{
    ENTRY_LOG;
    LOGINFO("DSPwrEventListener Destructor");
    Deinit();
    EXIT_LOG;
}

void DSPwrEventListener::Init(PluginHost::IShell* service)
{
    LOGINFO("DSPwrEventListener::Init - Entering");
    
    _service = service;
    _service->AddRef();
    
    // profileType is already initialized in DeviceSettingsImplementation.cpp constructor
    // No need to call searchRdkProfile() again here

    if (profileType == TV) { // TV
        if (DSProductTraits::UXController::Initialize(DSProductTraits::DEFAULT_TV_PROFILE)) {
            ux = DSProductTraits::UXController::GetInstance();
        }
    } else { // STB
        if (DSProductTraits::UXController::Initialize(DSProductTraits::DEFAULT_STB_PROFILE_EUROPE)) {
            ux = DSProductTraits::UXController::GetInstance();
        }
    }
    
    if (nullptr == ux) {
        LOGINFO("DSMgr product traits not supported");
    }

    try {
        device::Manager::load();
        LOGINFO("device::Manager::load success");
    } catch (...) {
        LOGERR("Exception Caught during device::Manager::load");
    }

    IARM_Result_t rc;
    rc = IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_SetStandbyVideoState, SetStandbyVideoState);
    if (IARM_RESULT_SUCCESS != rc) {
        LOGERR("IARM_Bus_RegisterCall Failed for SetStandbyVideoState, Error: %d", rc);
    }
    
    rc = IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_GetStandbyVideoState, GetStandbyVideoState);
    if (IARM_RESULT_SUCCESS != rc) {
        LOGERR("IARM_Bus_RegisterCall Failed for GetStandbyVideoState, Error: %d", rc);
    }
    
    rc = IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_SetAvPortState, SetAvPortState);
    if (IARM_RESULT_SUCCESS != rc) {
        LOGERR("IARM_Bus_RegisterCall Failed for SetAvPortState, Error: %d", rc);
    }
    
    rc = IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_SetLEDStatus, SetLEDState);
    if (IARM_RESULT_SUCCESS != rc) {
        LOGERR("IARM_Bus_RegisterCall Failed for SetLEDStatus, Error: %d", rc);
    }
    
    rc = IARM_Bus_RegisterCall(IARM_BUS_DSMGR_API_SetRebootConfig, SetRebootConfig);
    if (IARM_RESULT_SUCCESS != rc) {
        LOGERR("IARM_Bus_RegisterCall Failed for SetRebootConfig, Error: %d", rc);
    }

    // Initialize mutexes and condition variables
    pthread_mutex_init(&_pwrEventQueueMutexLock, NULL);
    pthread_mutex_init(&_pwrEventMutexLock, NULL);
    pthread_cond_init(&_pwrEventMutexCond, NULL);

    _stopThread = false;
    if (pthread_create(&_pwrEventHandlerThreadID, NULL, PwrEventHandlingThreadFunc, this) != 0) {
        LOGERR("DSMgr PwrEventHandlingThread creation failed");
    }

    // Initialize PowerManager connection
    InitializePowerManager();
    EXIT_LOG;
}

void DSPwrEventListener::Deinit()
{
    LOGINFO("DSPwrEventListener::Deinit - Entering");

    if (_powerManagerPlugin) {
        _powerManagerPlugin->Unregister(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
        _powerManagerPlugin.Reset();
    }
    _registeredPowerEventHandler = false;

    pthread_mutex_lock(&_pwrEventMutexLock);
    _stopThread = true;
    pthread_cond_signal(&_pwrEventMutexCond);
    pthread_mutex_unlock(&_pwrEventMutexLock);

    LOGINFO("Before joining thread");
    pthread_join(_pwrEventHandlerThreadID, NULL);
    LOGINFO("Completed joining thread");

    pthread_mutex_lock(&_pwrEventQueueMutexLock);
    while (!_pwrEventQueue.empty()) {
        _pwrEventQueue.pop();
    }
    pthread_mutex_unlock(&_pwrEventQueueMutexLock);

    pthread_cond_destroy(&_pwrEventMutexCond);
    pthread_mutex_destroy(&_pwrEventQueueMutexLock);
    pthread_mutex_destroy(&_pwrEventMutexLock);

    if (_service) {
        _service->Release();
        _service = nullptr;
    }
    EXIT_LOG;
}

void DSPwrEventListener::InitializePowerManager()
{
    ENTRY_LOG;
    LOGINFO("InitializePowerManager - Connecting to PowerManager plugin");
    PowerState pwrStateCur = PowerState::POWER_STATE_UNKNOWN;
    PowerState pwrStatePrev = PowerState::POWER_STATE_UNKNOWN;
    Core::hresult retStatus = Core::ERROR_GENERAL;
    
    _powerManagerPlugin = PowerManagerInterfaceBuilder(_T("org.rdk.PowerManager"))
        .withIShell(_service)
        .withRetryIntervalMS(200)
        .withRetryCount(25)
        .createInterface();

    registerPowerEventHandler();

    if (_powerManagerPlugin) {
        retStatus = _powerManagerPlugin->GetPowerState(pwrStateCur, pwrStatePrev);
    }
    
    if (Core::ERROR_NONE == retStatus) {
        _curState = pwrStateCur;
        LOGINFO("InitializePowerManager - Current power state: %d", _curState);
        PwrControllerFetchNinitStateValues();
    } else {
        LOGERR("InitializePowerManager - Failed to get power state");
    }
    EXIT_LOG;
}

void DSPwrEventListener::registerPowerEventHandler()
{
    ENTRY_LOG;
    if (!_registeredPowerEventHandler && _powerManagerPlugin) {
        LOGINFO("Registering PowerManager event handler");
        _registeredPowerEventHandler = true;
        _powerManagerPlugin->Register(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
    } else {
        LOGINFO("PowerManager event handler already registered or plugin not available");
    }
    EXIT_LOG;
}

void PowerManagerNotification::OnPowerModeChanged(const PowerState currentState, const PowerState newState)
{
    _parent.onPowerModeChanged(currentState, newState);
}

void DSPwrEventListener::onPowerModeChanged(const PowerState currentState, const PowerState newState)
{
    ENTRY_LOG;
    LOGINFO("DSPwrEventListener::onPowerModeChanged - currentState: %d, newState: %d", currentState, newState);
    
    #if 0
    pthread_mutex_lock(&_pwrEventQueueMutexLock);
    _pwrEventQueue.emplace(currentState, newState);
    pthread_mutex_unlock(&_pwrEventQueueMutexLock);
    
    LOGINFO("Sending signal to thread for processing callback event");
    pthread_mutex_lock(&_pwrEventMutexLock);
    pthread_cond_signal(&_pwrEventMutexCond);
    pthread_mutex_unlock(&_pwrEventMutexLock);
        if (_service) {
        _service->Release();
        _service = nullptr;
    }
    #endif
    HandlePwrEventData(currentState, newState);
    EXIT_LOG;
}

void DSPwrEventListener::PwrCtrlEstablishConnection()
{
    ENTRY_LOG;
    LOGINFO("DSPwrEventListener::PwrCtrlEstablishConnection - Using PowerManager plugin");
    // PowerManager connection is now handled in InitializePowerManager
    // This method is kept for compatibility but does nothing
    EXIT_LOG;
}

void* DSPwrEventListener::PwrRetryEstablishConnThread(void* arg)
{
    ENTRY_LOG;
    LOGINFO("PwrRetryEstablishConnThread: Not used with PowerManager plugin");
    // This thread is no longer needed with PowerManager plugin
    EXIT_LOG;
    return arg;
}

void DSPwrEventListener::PwrControllerFetchNinitStateValues()
{
    ENTRY_LOG;
    LOGINFO("DSPwrEventListener::PwrControllerFetchNinitStateValues");
    
    PowerState powerStateBeforeReboot = PowerState::POWER_STATE_STANDBY;

    // Note: _curState is already set in InitializePowerManager from GetPowerState
    LOGINFO("Current Power State: %d", _curState);

    if (nullptr != ux) {
        ux->ApplyPostRebootConfig(_curState, powerStateBeforeReboot);
    }

    if (nullptr == ux) {
#ifndef DISABLE_LED_SYNC_IN_BOOTUP
        SetLEDStatus(_curState);
#endif
        SetAVPortsPowerState(_curState);
    }
    EXIT_LOG;
}

void* DSPwrEventListener::PwrEventHandlingThreadFunc(void* arg)
{
#if 0
    LOGINFO("PwrEventHandlingThreadFunc: Entry");
    DSPwrEventListener* listener = static_cast<DSPwrEventListener*>(arg);

    while (true) {
        pthread_mutex_lock(&listener->_pwrEventMutexLock);
        LOGINFO("PwrEventHandlingThreadFunc... Wait for Events");
        
        pthread_mutex_lock(&listener->_pwrEventQueueMutexLock);
        bool queueEmpty = listener->_pwrEventQueue.empty();
        pthread_mutex_unlock(&listener->_pwrEventQueueMutexLock);
        
        while (!listener->_stopThread && queueEmpty) {
            pthread_cond_wait(&listener->_pwrEventMutexCond, &listener->_pwrEventMutexLock);
            pthread_mutex_lock(&listener->_pwrEventQueueMutexLock);
            queueEmpty = listener->_pwrEventQueue.empty();
            pthread_mutex_unlock(&listener->_pwrEventQueueMutexLock);
        }
        
        if (listener->_stopThread) {
            LOGINFO("PwrEventHandlingThreadFunc Exiting due to stop thread");
            pthread_mutex_unlock(&listener->_pwrEventMutexLock);
            break;
        }
        pthread_mutex_unlock(&listener->_pwrEventMutexLock);

        pthread_mutex_lock(&listener->_pwrEventQueueMutexLock);
        while (!listener->_pwrEventQueue.empty()) {
            DSMgr_Power_Event_State_t pwrEvent = listener->_pwrEventQueue.front();
            listener->_pwrEventQueue.pop();
            pthread_mutex_unlock(&listener->_pwrEventQueueMutexLock);
            
            listener->HandlePwrEventData(pwrEvent.currentState, pwrEvent.newState);
            
            pthread_mutex_lock(&listener->_pwrEventQueueMutexLock);
        }
        pthread_mutex_unlock(&listener->_pwrEventQueueMutexLock);
    }
#endif
    return arg;
}

void DSPwrEventListener::HandlePwrEventData(const PowerState currentState,
                                           const PowerState newState)
{
    ENTRY_LOG;
    LOGINFO("HandlePwrEventData - currentState: %d, newState: %d", currentState, newState);
    
    if (nullptr != ux) {
        ux->ApplyPowerStateChangeConfig(newState, currentState);
    } else {
#ifndef DISABLE_LED_SYNC_IN_BOOTUP
        SetLEDStatus(newState);
#endif
        SetAVPortsPowerState(newState);
    }
    EXIT_LOG;
}

int DSPwrEventListener::SetLEDStatus(PowerState powerState)
{
    ENTRY_LOG;
    LOGINFO("SetLEDStatus - powerState: %d", powerState);
    
    try {
        dsFPDStateParam_t param;
        param.eIndicator = dsFPD_INDICATOR_POWER;

        if (PowerState::POWER_STATE_ON != powerState) {
            if (profileType == TV) { // TV
                param.state = dsFPD_STATE_ON;
                LOGINFO("Settings Power LED State to ON");
            } else {
                param.state = dsFPD_STATE_OFF;
                LOGINFO("Settings Power LED State to OFF");
            }
        } else {
            param.state = dsFPD_STATE_ON;
            LOGINFO("Settings Power LED State to ON");
        }

        _dsSetFPState(&param);
    } catch (...) {
        LOGERR("Exception Caught during SetLEDStatus");
        EXIT_LOG;
        return 0;
    }
    EXIT_LOG;
    return 0;
}

int DSPwrEventListener::SetAVPortsPowerState(PowerState powerState)
{
    ENTRY_LOG;
    LOGINFO("SetAVPortsPowerState - powerState: %d", powerState);
    
    try {
        if (PowerState::POWER_STATE_ON != powerState) {
            // Handle video ports
            /*try {
                device::List<device::VideoOutputPort> videoPorts = device::Host::getInstance().getVideoOutputPorts();
                LOGINFO("Number of Video Ports: %zu", videoPorts.size());
                
                for (size_t i = 0; i < videoPorts.size(); i++) {
                    try {
                        device::VideoOutputPort vPort = videoPorts.at(i);
                        bool doEnable = GetVideoPortStandbySetting(vPort.getName().c_str());
                        
                        if ((false == doEnable) || (PowerState::POWER_STATE_OFF == powerState)) {
                            // Disable the port
                            // Implementation would call RPC methods
                            LOGINFO("VideoPort %s disabled for powerState %d", vPort.getName().c_str(), powerState);
                        } else {
                            LOGINFO("Disable VideoPort %s skipped", vPort.getName().c_str());
                        }
                    } catch (...) {
                        LOGERR("Video port exception at %zu", i);
                    }
                }
            } catch (...) {
                LOGERR("Video port exception");
            }*/

            // Handle audio ports
            /*try {
                device::List<device::AudioOutputPort> aPorts = device::Host::getInstance().getAudioOutputPorts();
                LOGINFO("Number of Audio Ports: %zu", aPorts.size());
                
                for (size_t i = 0; i < aPorts.size(); i++) {
                    try {
                        device::AudioOutputPort aPort = aPorts.at(i);
                        // Disable audio port
                        LOGINFO("AudioPort %s disabled for powerState %d", aPort.getName().c_str(), powerState);
                    } catch (...) {
                        LOGERR("Audio port exception at %zu", i);
                    }
                }
            } catch (...) {
                LOGERR("Audio port exception");
            }*/
        } else {
            // Power ON - Enable ports
            /*try {
                device::List<device::VideoOutputPort> videoPorts = device::Host::getInstance().getVideoOutputPorts();
                for (size_t i = 0; i < videoPorts.size(); i++) {
                    try {
                        device::VideoOutputPort vPort = videoPorts.at(i);
                        LOGINFO("VideoPort %s enabled for powerState %d", vPort.getName().c_str(), powerState);
                    } catch (...) {
                        LOGERR("Video port exception at %zu", i);
                    }
                }

                device::List<device::AudioOutputPort> aPorts = device::Host::getInstance().getAudioOutputPorts();
                for (size_t i = 0; i < aPorts.size(); i++) {
                    try {
                        device::AudioOutputPort aPort = aPorts.at(i);
                        LOGINFO("AudioPort %s enabled for powerState %d", aPort.getName().c_str(), powerState);
                    } catch (...) {
                        LOGERR("Audio port exception at %zu", i);
                    }
                }
                
                if (DSController::instance()->getEASMode() == IARM_BUS_SYS_MODE_EAS) {
                    LOGINFO("Force Stereo in EAS mode");
                    _setEASAudioMode();
                }
            } catch (...) {
                LOGERR("Exception during power ON");
            }*/
        }
    } catch (...) {
        LOGERR("Exception Caught during SetAVPortsPowerState");
        EXIT_LOG;
        return 0;
    }
    
    LOGINFO("Exiting SetAVPortsPowerState");
    EXIT_LOG;
    return 0;
}

bool DSPwrEventListener::GetVideoPortStandbySetting(const char* port)
{
    ENTRY_LOG;
    if (NULL == port) {
        LOGERR("Port name is NULL");
        EXIT_LOG;
        return false;
    }
    
    for (int i = 0; i < MAX_NUM_VIDEO_PORTS; i++) {
        if (0 == strncasecmp(port, _standbyVideoPortSetting[i].port, DSMGR_MAX_VIDEO_PORT_NAME_LENGTH)) {
            EXIT_LOG;
            return _standbyVideoPortSetting[i].isEnabled;
        }
    }
    EXIT_LOG;
    return false; // Default: video port is disabled in standby mode
}

IARM_Result_t DSPwrEventListener::SetStandbyVideoState(void* arg)
{
    ENTRY_LOG;
    if (NULL == arg) {
        EXIT_LOG;
        return IARM_RESULT_INVALID_PARAM;
    }
    
    if (!_instance) {
        EXIT_LOG;
        return IARM_RESULT_INVALID_STATE;
    }
    
    dsMgrStandbyVideoStateParam_t* param = (dsMgrStandbyVideoStateParam_t*)arg;
    param->result = 0;

    int i = 0;
    for (i = 0; i < MAX_NUM_VIDEO_PORTS; i++) {
        if (0 == strncasecmp(param->port, _instance->_standbyVideoPortSetting[i].port, DSMGR_MAX_VIDEO_PORT_NAME_LENGTH)) {
            _instance->_standbyVideoPortSetting[i].isEnabled = ((0 == param->isEnabled) ? false : true);
            break;
        }
    }
    
    if (MAX_NUM_VIDEO_PORTS == i) {
        for (i = 0; i < MAX_NUM_VIDEO_PORTS; i++) {
            if ('\0' == _instance->_standbyVideoPortSetting[i].port[0]) {
                strncpy(_instance->_standbyVideoPortSetting[i].port, param->port, (DSMGR_MAX_VIDEO_PORT_NAME_LENGTH - 1));
                _instance->_standbyVideoPortSetting[i].isEnabled = ((0 == param->isEnabled) ? false : true);
                break;
            }
        }
    }
    
    EXIT_LOG;
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t DSPwrEventListener::GetStandbyVideoState(void* arg)
{
    ENTRY_LOG;
    if (NULL == arg) {
        EXIT_LOG;
        return IARM_RESULT_INVALID_PARAM;
    }
    
    if (!_instance) {
        EXIT_LOG;
        return IARM_RESULT_INVALID_STATE;
    }
    
    dsMgrStandbyVideoStateParam_t* param = (dsMgrStandbyVideoStateParam_t*)arg;
    param->isEnabled = (_instance->GetVideoPortStandbySetting(param->port) ? 1 : 0);
    param->result = 0;
    
    EXIT_LOG;
    return IARM_RESULT_SUCCESS;
}

#if 0
PowerState DSPwrEventListener::PwrMgrToPowerControllerPowerState(int pwrMgrState)
{
    PowerState powerState = PowerState::POWER_STATE_UNKNOWN;
    
    switch (pwrMgrState) {
        case 0: // PWRMGR_POWERSTATE_OFF
            powerState = PowerState::POWER_STATE_OFF;
            break;
        case 1: // PWRMGR_POWERSTATE_STANDBY
            powerState = PowerState::POWER_STATE_STANDBY;
            break;
        case 2: // PWRMGR_POWERSTATE_ON
            powerState = PowerState::POWER_STATE_ON;
            break;
        case 3: // PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP
            powerState = PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP;
            break;
        case 4: // PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP
            powerState = PowerState::POWER_STATE_STANDBY_DEEP_SLEEP;
            break;
        default:
            LOGERR("Invalid Power State: %d", pwrMgrState);
            break;
    }
    
    return powerState;
}
#endif

IARM_Result_t DSPwrEventListener::SetAvPortState(void* arg)
{
    ENTRY_LOG;
    if (NULL == arg || !_instance) {
        EXIT_LOG;
        return IARM_RESULT_INVALID_PARAM;
    }
    
    dsMgrAVPortStateParam_t* param = (dsMgrAVPortStateParam_t*)arg;
    PowerState powerState = (WPEFramework::Exchange::IPowerManager::PowerState) param->avPortPowerState;
    
    if (PowerState::POWER_STATE_UNKNOWN != powerState) {
        _instance->SetAVPortsPowerState(powerState);
    }
    
    param->result = 0;
    EXIT_LOG;
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t DSPwrEventListener::SetLEDState(void* arg)
{
    ENTRY_LOG;
    if (NULL == arg || !_instance) {
        EXIT_LOG;
        return IARM_RESULT_INVALID_PARAM;
    }
    
    dsMgrLEDStatusParam_t* param = (dsMgrLEDStatusParam_t*)arg;
    PowerState powerState = (WPEFramework::Exchange::IPowerManager::PowerState) param->ledState;
    
    if (PowerState::POWER_STATE_UNKNOWN != powerState) {
        _instance->SetLEDStatus(powerState);
    }
    
    param->result = 0;
    EXIT_LOG;
    return IARM_RESULT_SUCCESS;
}

IARM_Result_t DSPwrEventListener::SetRebootConfig(void* arg)
{
    ENTRY_LOG;
    if (NULL == arg) {
        EXIT_LOG;
        return IARM_RESULT_INVALID_PARAM;
    }
    
    dsMgrRebootConfigParam_t* param = (dsMgrRebootConfigParam_t*)arg;
    param->reboot_reason_custom[sizeof(param->reboot_reason_custom) - 1] = '\0';
    
    if (nullptr != ux) {
        PowerState powerState = (WPEFramework::Exchange::IPowerManager::PowerState) param->powerState;
        
        if (PowerState::POWER_STATE_UNKNOWN != powerState) {
            if (0 == strncmp(PWRMGR_REBOOT_REASON_MAINTENANCE, param->reboot_reason_custom, 
                            sizeof(param->reboot_reason_custom))) {
                ux->ApplyPreMaintenanceRebootConfig(powerState);
            } else {
                ux->ApplyPreRebootConfig(powerState);
            }
        }
    }
    
    param->result = 0;
    EXIT_LOG;
    return IARM_RESULT_SUCCESS;
}

} // namespace Plugin
} // namespace WPEFramework
