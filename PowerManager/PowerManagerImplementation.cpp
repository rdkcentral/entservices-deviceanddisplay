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

#include <stdlib.h>
#include <errno.h>
#include <string>
#include <iomanip>
#include <sys/prctl.h>
#include <mutex>

#include "PowerManagerImplementation.h"
#include "UtilsIarm.h"
#include "UtilsProcess.h"
#include "rdk/iarmmgrs-hal/pwrMgr.h"
#include "host.hpp"


#define STANDBY_REASON_FILE                     "/opt/standbyReason.txt"
#define IARM_BUS_PWRMGR_API_SetDeepSleepTimeOut	"SetDeepSleepTimeOut" /*!< Sets the timeout for deep sleep*/

using namespace std;
string convertCase(string str)
{
    std::string bufferString = str;
    transform(bufferString.begin(), bufferString.end(),
            bufferString.begin(), ::toupper);
    LOGINFO("%s: after transform to upper :%s", __FUNCTION__,
            bufferString.c_str());
    return bufferString;
}

bool convert(string str3, string firm)
{
    LOGINFO("INSIDE CONVERT");
    bool status = false;
    string firmware = convertCase(firm);
    string str = firmware.c_str();
    size_t found = str.find(str3);
    if (found != string::npos) {
        status = true;
    } else {
        status = false;
    }
    return status;
}

namespace WPEFramework {
namespace Plugin {
    static void _powerEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

    SERVICE_REGISTRATION(PowerManagerImplementation, 1, 0);
    PowerManagerImplementation* PowerManagerImplementation::_instance = nullptr;
    PowerManagerImplementation::PowerManagerImplementation()
    {
        PowerManagerImplementation::_instance = this;
        m_networkStandbyModeValid = false;
        m_powerStateBeforeReboot = POWER_STATE_UNKNOWN;

#if defined(USE_IARMBUS) || defined(USE_IARM_BUS)
        InitializeIARM();
#endif /* defined(USE_IARMBUS) || defined(USE_IARM_BUS) */
    }

    PowerManagerImplementation::~PowerManagerImplementation()
    {
        DeinitializeIARM();
    }

#if defined(USE_IARMBUS) || defined(USE_IARM_BUS)
        void PowerManagerImplementation::InitializeIARM()
        {
            if (Utils::IARM::init())
            {
                IARM_Result_t res = IARM_RESULT_INVALID_PARAM;
                IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, _powerEventHandler));
                IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_REBOOTING, _powerEventHandler));
                IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED, _powerEventHandler));
                IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_DEEPSLEEP_TIMEOUT,_powerEventHandler ));

#ifdef ENABLE_THERMAL_PROTECTION
                IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_THERMAL_MODECHANGED, _powerEventHandler));
#endif //ENABLE_THERMAL_PROTECTION
            }
	    
        }

        void PowerManagerImplementation::DeinitializeIARM()
        {
            if (Utils::IARM::isConnected())
            {
                IARM_Result_t res = IARM_RESULT_INVALID_PARAM;
                IARM_CHECK( IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, _powerEventHandler));
                IARM_CHECK( IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_REBOOTING, _powerEventHandler));
                IARM_CHECK( IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED,_powerEventHandler ));
                IARM_CHECK( IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_DEEPSLEEP_TIMEOUT,_powerEventHandler ));

#ifdef ENABLE_THERMAL_PROTECTION
                IARM_CHECK( IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_THERMAL_MODECHANGED, _powerEventHandler));
#endif //ENABLE_THERMAL_PROTECTION
            }
        }
#endif /* defined(USE_IARMBUS) || defined(USE_IARM_BUS) */

        void PowerManagerImplementation::dispatchPowerModeChangedEvent(const PowerState &currentState, const PowerState &newState)
        {
            std::list<Exchange::IPowerManager::INotification*>::const_iterator index(_powerManagerNotification.begin());
            while (index != _powerManagerNotification.end())
            {
                (*index)->OnPowerModeChanged(currentState,newState);
                ++index;
            }
        }

        void PowerManagerImplementation::dispatchPowerModePreChangeEvent(const PowerState &currentState, const PowerState &newState)
        {
            std::list<Exchange::IPowerManager::INotification*>::const_iterator index(_powerManagerNotification.begin());
            while (index != _powerManagerNotification.end())
            {
                (*index)->OnPowerModePreChange(currentState,newState);
                ++index;
            }
        }

        void PowerManagerImplementation::dispatchDeepSleepTimeoutEvent(const uint32_t &timeout)
        {
            std::list<Exchange::IPowerManager::INotification*>::const_iterator index(_powerManagerNotification.begin());
            while (index != _powerManagerNotification.end())
            {
                (*index)->OnDeepSleepTimeout(timeout);
                ++index;
            }
        }

        void PowerManagerImplementation::dispatchRebootBeginEvent(const string &rebootReasonCustom, const string &rebootReasonOther, const string &rebootRequestor)
        {
            std::list<Exchange::IPowerManager::INotification*>::const_iterator index(_powerManagerNotification.begin());
            while (index != _powerManagerNotification.end())
            {
                (*index)->OnRebootBegin(rebootReasonCustom, rebootReasonOther, rebootRequestor);
                ++index;
            }
        }

        void PowerManagerImplementation::dispatchThermalModeChangedEvent(const ThermalTemperature &currentThermalLevel, const ThermalTemperature &newThermalLevel, const float &currentTemperature)
        {
            std::list<Exchange::IPowerManager::INotification*>::const_iterator index(_powerManagerNotification.begin());
            while (index != _powerManagerNotification.end())
            {
                (*index)->OnThermalModeChanged(currentThermalLevel, newThermalLevel, currentTemperature);
                ++index;
            }
        }

        void PowerManagerImplementation::dispatchNetworkStandbyModeChangedEvent(const bool &enabled)
        {
            std::list<Exchange::IPowerManager::INotification*>::const_iterator index(_powerManagerNotification.begin());
            while (index != _powerManagerNotification.end())
            {
                (*index)->OnNetworkStandbyModeChanged(enabled);
                ++index;
            }
        }

        void PowerManagerImplementation::Dispatch(Event event, ParamsType params)
        {
            _adminLock.Lock();

            switch (event)
            {
                case  PWRMGR_EVENT_POWERMODE_CHANGED:
                {
                   if (const auto* pairValue = boost::get<std::pair<PowerState, PowerState>>(&params))
                   {
                       PowerState currentState = pairValue->first,newState = pairValue->second;
                      dispatchPowerModeChangedEvent(currentState, newState);
                   }
                }
                    break;

                case  PWRMGR_EVENT_POWERMODE_PRECHANGE:
                {
                   if (const auto* pairValue = boost::get<std::pair<PowerState, PowerState>>(&params))
                   {
                       PowerState currentState = pairValue->first,newState = pairValue->second;
                       dispatchPowerModePreChangeEvent(currentState, newState);
                   }
                }
                    break;

                case  PWRMGR_EVENT_DEEPSLEEP_TIMEOUT:
                {
                    if (const uint32_t* value = boost::get<uint32_t>(&params))
                   {
                       dispatchDeepSleepTimeoutEvent(*value);
                   }
                }
                    break;

                case  PWRMGR_EVENT_REBOOTING:
                {
                   if (const auto* tupleValue = boost::get<std::tuple<std::string, std::string, std::string>>(&params))
                   {
                       string rebootReasonCustom = std::get<0>(*tupleValue), rebootReasonOther = std::get<1>(*tupleValue), rebootRequestor = std::get<2>(*tupleValue);
                       dispatchRebootBeginEvent(rebootReasonCustom, rebootReasonOther, rebootRequestor);
                   }
                }
                    break;

                case  PWRMGR_EVENT_THERMAL_MODECHANGED:
                {
                   if (const auto* thermData = boost::get<std::tuple<ThermalTemperature, ThermalTemperature, float>>(&params))
                   {
                       ThermalTemperature currentThermalLevel = std::get<0>(*thermData), newThermalLevel = std::get<1>(*thermData);
                       float currentTemperature = std::get<2>(*thermData);
                       dispatchThermalModeChangedEvent(currentThermalLevel, newThermalLevel, currentTemperature);
                   }
                }
                    break;

                case  PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED:
                {
                    if (const bool* boolValue = boost::get<bool>(&params))
                    {
                         dispatchNetworkStandbyModeChangedEvent(*boolValue);
                    }
                }
                    break;
            }

            _adminLock.Unlock();
        }

        void _powerEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            switch (eventId)
            {
                case  IARM_BUS_PWRMGR_EVENT_MODECHANGED:
                {
                    IARM_Bus_PWRMgr_EventData_t *eventData = (IARM_Bus_PWRMgr_EventData_t *)data;
                    auto pairParam = std::make_pair(PowerManagerImplementation::_instance->ConvertToPwrMgrPowerState(eventData->data.state.curState),
                                                            PowerManagerImplementation::_instance->ConvertToPwrMgrPowerState(eventData->data.state.newState));

                    LOGINFO("IARM Event triggered for PowerStateChange. Old State %u, New State: %u",
                                eventData->data.state.curState,eventData->data.state.newState);

                    Core::IWorkerPool::Instance().Submit(PowerManagerImplementation::Job::Create(PowerManagerImplementation::_instance,
                                                PowerManagerImplementation::PWRMGR_EVENT_POWERMODE_CHANGED,
                                                pairParam));
                }
                    break;

                case  IARM_BUS_PWRMGR_EVENT_REBOOTING:
                {
                    IARM_Bus_PWRMgr_RebootParam_t *eventData = (IARM_Bus_PWRMgr_RebootParam_t *)data;
                    auto tupleParam = std::make_tuple(eventData->reboot_reason_custom,
                                                eventData->reboot_reason_other,
                                                eventData->requestor);

                    LOGINFO("IARM Event triggered for reboot. reboot_reason_custom: %s, requestor: %s, reboot_reason_other: %s",
                                eventData->reboot_reason_custom, eventData->requestor, eventData->reboot_reason_other);

                    Core::IWorkerPool::Instance().Submit(PowerManagerImplementation::Job::Create(PowerManagerImplementation::_instance,
                                                PowerManagerImplementation::PWRMGR_EVENT_REBOOTING,
                                                tupleParam));
                }
                    break;

                case  IARM_BUS_PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED:
                {
                    IARM_Bus_PWRMgr_EventData_t *eventData = (IARM_Bus_PWRMgr_EventData_t *)data;

                    LOGINFO("IARM Event triggered for NetworkStandbyMode. NetworkStandbyMode: %u", eventData->data.bNetworkStandbyMode);

                    Core::IWorkerPool::Instance().Submit(PowerManagerImplementation::Job::Create(PowerManagerImplementation::_instance,
                                                PowerManagerImplementation::PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED,
                                                eventData->data.bNetworkStandbyMode));
                }
                    break;

                case  IARM_BUS_PWRMGR_EVENT_THERMAL_MODECHANGED:
                {
                    IARM_Bus_PWRMgr_EventData_t *eventData = (IARM_Bus_PWRMgr_EventData_t *)data;
                    auto tupleParam = std::make_tuple(PowerManagerImplementation::_instance->ConvertToThermalState(eventData->data.therm.curLevel),
                                        PowerManagerImplementation::_instance->ConvertToThermalState(eventData->data.therm.newLevel),
                                        (int)eventData->data.therm.curTemperature);

                    Core::IWorkerPool::Instance().Submit(PowerManagerImplementation::Job::Create(PowerManagerImplementation::_instance,
                                                PowerManagerImplementation::PWRMGR_EVENT_THERMAL_MODECHANGED,
                                                tupleParam));

                    LOGINFO("THERMAL_MODECHANGED event received, curLevel: %u, newLevel: %u, curTemperature: %f",
						eventData->data.therm.curLevel,eventData->data.therm.newLevel,eventData->data.therm.curTemperature);
                }
                    break;

                case  IARM_BUS_PWRMGR_EVENT_DEEPSLEEP_TIMEOUT:
                {
                    IARM_BUS_PWRMgr_DeepSleepTimeout_EventData_t *eventData = (IARM_BUS_PWRMgr_DeepSleepTimeout_EventData_t *)data;

                    Core::IWorkerPool::Instance().Submit(PowerManagerImplementation::Job::Create(PowerManagerImplementation::_instance,
                                                PowerManagerImplementation::PWRMGR_EVENT_DEEPSLEEP_TIMEOUT,
                                                eventData->timeout));

                    LOGINFO("IARM Event triggered for deep sleep timeout: %u", eventData->timeout);
                }
                    break;

            }
        }

    uint32_t PowerManagerImplementation::Register(Exchange::IPowerManager::INotification *notification)
    {
        ASSERT (nullptr != notification);

        _adminLock.Lock();

        // Make sure we can't register the same notification callback multiple times
        if (std::find(_powerManagerNotification.begin(), _powerManagerNotification.end(), notification) == _powerManagerNotification.end())
        {
            //LOGINFO("Register notification");
            _powerManagerNotification.push_back(notification);
            notification->AddRef();
        }

        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    uint32_t PowerManagerImplementation::Unregister(Exchange::IPowerManager::INotification *notification )
    {
        uint32_t status = Core::ERROR_GENERAL;

        ASSERT (nullptr != notification);

        _adminLock.Lock();

        // Make sure we can't unregister the same notification callback multiple times
        auto itr = std::find(_powerManagerNotification.begin(), _powerManagerNotification.end(), notification);
        if (itr != _powerManagerNotification.end())
        {
            (*itr)->Release();
            //LOGINFO("Unregister notification");
            _powerManagerNotification.erase(itr);
            status = Core::ERROR_NONE;
        }
        else
        {
            //LOGERR("notification not found");
        }

        _adminLock.Unlock();

        return status;
    }

    IARM_Bus_PWRMgr_PowerState_t PowerManagerImplementation::ConvertToIarmBusPowerState(PowerState state)
    {
        IARM_Bus_PWRMgr_PowerState_t powerState = IARM_BUS_PWRMGR_POWERSTATE_OFF;

        switch (state) {
            case POWER_STATE_OFF:
                powerState = IARM_BUS_PWRMGR_POWERSTATE_OFF;
            break;
            case POWER_STATE_STANDBY:
                powerState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY;
            break;
            case POWER_STATE_ON:
                powerState = IARM_BUS_PWRMGR_POWERSTATE_ON;
            break;
            case POWER_STATE_STANDBY_LIGHT_SLEEP:
                powerState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP;
            break;
            case POWER_STATE_STANDBY_DEEP_SLEEP:
                powerState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP;
            break;
            default:
                powerState = IARM_BUS_PWRMGR_POWERSTATE_OFF;
            break;
        }

        return powerState;
    }

    PowerState PowerManagerImplementation::ConvertToPwrMgrPowerState(IARM_Bus_PWRMgr_PowerState_t state)
    {
        PowerState powerState;

        switch (state) {
            case IARM_BUS_PWRMGR_POWERSTATE_OFF:
                powerState = POWER_STATE_OFF;
            break;
            case IARM_BUS_PWRMGR_POWERSTATE_STANDBY:
                powerState = POWER_STATE_STANDBY;
            break;
            case IARM_BUS_PWRMGR_POWERSTATE_ON:
                powerState = POWER_STATE_ON;
            break;
            case IARM_BUS_PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP:
                powerState = POWER_STATE_STANDBY_LIGHT_SLEEP;
            break;
            case IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP:
                powerState = POWER_STATE_STANDBY_DEEP_SLEEP;
            break;
            default:
                powerState = POWER_STATE_UNKNOWN;
            break;
        }

        return powerState;
    }

    WakeupReason PowerManagerImplementation::ConvertToDeepSleepWakeupReason(DeepSleep_WakeupReason_t deepSleepWakeupReason)
    {
        WakeupReason wakeupReason;

        switch(deepSleepWakeupReason)
        {
            case DEEPSLEEP_WAKEUPREASON_IR:
                wakeupReason = WAKEUP_REASON_IR;
            break;
            case DEEPSLEEP_WAKEUPREASON_RCU_BT:
                wakeupReason = WAKEUP_REASON_BLUETOOTH;
            break;
            case DEEPSLEEP_WAKEUPREASON_RCU_RF4CE:
                wakeupReason = WAKEUP_REASON_RF4CE;
            break;
            case DEEPSLEEP_WAKEUPREASON_GPIO:
                wakeupReason = WAKEUP_REASON_GPIO;
            break;
            case DEEPSLEEP_WAKEUPREASON_LAN:
                wakeupReason = WAKEUP_REASON_LAN;
            break;
            case DEEPSLEEP_WAKEUPREASON_WLAN:
                wakeupReason = WAKEUP_REASON_WIFI;
            break;
            case DEEPSLEEP_WAKEUPREASON_TIMER:
                wakeupReason = WAKEUP_REASON_TIMER;
            break;
            case DEEPSLEEP_WAKEUPREASON_FRONT_PANEL:
                wakeupReason = WAKEUP_REASON_FRONTPANEL;
            break;
            case DEEPSLEEP_WAKEUPREASON_WATCHDOG:
                wakeupReason = WAKEUP_REASON_WATCHDOG;
            break;
            case DEEPSLEEP_WAKEUPREASON_SOFTWARE_RESET:
                wakeupReason = WAKEUP_REASON_SOFTWARERESET;
            break;
            case DEEPSLEEP_WAKEUPREASON_THERMAL_RESET:
                wakeupReason = WAKEUP_REASON_THERMALRESET;
            break;
            case DEEPSLEEP_WAKEUPREASON_WARM_RESET:
                wakeupReason = WAKEUP_REASON_WARMRESET;
            break;
            case DEEPSLEEP_WAKEUPREASON_COLDBOOT:
                wakeupReason = WAKEUP_REASON_COLDBOOT;
            break;
            case DEEPSLEEP_WAKEUPREASON_STR_AUTH_FAILURE:
                wakeupReason = WAKEUP_REASON_STRAUTHFAIL;
            break;
            case DEEPSLEEP_WAKEUPREASON_CEC:
                wakeupReason = WAKEUP_REASON_CEC;
            break;
            case DEEPSLEEP_WAKEUPREASON_PRESENCE:
                wakeupReason = WAKEUP_REASON_PRESENCE;
            break;
            case DEEPSLEEP_WAKEUPREASON_VOICE:
                wakeupReason = WAKEUP_REASON_VOICE;
            break;
            default:
                wakeupReason = WAKEUP_REASON_UNKNOWN;
            break;
        }

        return wakeupReason;
    }

    ThermalTemperature PowerManagerImplementation::ConvertToThermalState(IARM_Bus_PWRMgr_ThermalState_t thermalState)
    {
        ThermalTemperature pwrMgrThermState;

        switch(thermalState) {
            case IARM_BUS_PWRMGR_TEMPERATURE_NORMAL:
                pwrMgrThermState = THERMAL_TEMPERATURE_NORMAL;
            break;
            case IARM_BUS_PWRMGR_TEMPERATURE_HIGH:
                pwrMgrThermState = THERMAL_TEMPERATURE_HIGH;
            break;
            case IARM_BUS_PWRMGR_TEMPERATURE_CRITICAL:
                pwrMgrThermState = THERMAL_TEMPERATURE_CRITICAL;
            break;
            default:
                pwrMgrThermState = THERMAL_TEMPERATURE_UNKNOWN;
            break;
        }

        return pwrMgrThermState;
    }

    IARM_Bus_Daemon_SysMode_t PowerManagerImplementation::ConvertToDaemonSystemMode(SystemMode sysMode)
    {
        IARM_Bus_Daemon_SysMode_t systemMode = IARM_BUS_SYS_MODE_NORMAL;

        switch(sysMode) {
            case SYSTEM_MODE_NORMAL:
                systemMode = IARM_BUS_SYS_MODE_NORMAL;
            break;
            case SYSTEM_MODE_EAS:
                systemMode = IARM_BUS_SYS_MODE_EAS;
            break;
            case SYSTEM_MODE_WAREHOUSE:
                systemMode = IARM_BUS_SYS_MODE_WAREHOUSE;
            break;
            default:
                systemMode = IARM_BUS_SYS_MODE_NORMAL;
            break;
        }

        return systemMode;
    }

    uint32_t PowerManagerImplementation::GetPowerState(PowerState& currentState, PowerState& prevState ) const
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_GetPowerState_Param_t param = {};

        LOGINFO("impl called for GetPowerState()");
        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_API_GetPowerState,
            (void*)&param, sizeof(param));

        if (res == IARM_RESULT_SUCCESS)
        {
            currentState = PowerManagerImplementation::_instance->ConvertToPwrMgrPowerState(param.curState);
            prevState = PowerManagerImplementation::_instance->ConvertToPwrMgrPowerState(param.prevState);
            errorCode = Core::ERROR_NONE;
        }

        LOGINFO("getPowerState called, currentState : %u, prevState : %u", currentState,prevState);

        return errorCode;
    }

    uint32_t PowerManagerImplementation::SetDevicePowerState(const int &keyCode, PowerState powerState)
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_SetPowerState_Param_t param = {};

        param.newState = PowerManagerImplementation::_instance->ConvertToIarmBusPowerState(powerState);

        if (POWER_STATE_UNKNOWN != powerState)
        {
            param.keyCode = keyCode;
            IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_API_SetPowerState,
                (void*)&param, sizeof(param));

            if (res == IARM_RESULT_SUCCESS)
            {
                errorCode = Core::ERROR_NONE;
            }
        }
        else
        {
            LOGWARN("Invalid power state is received %u", powerState);
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::SetPowerState(const int &keyCode, const PowerState &powerState,const string &standbyReason)
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        string sleepMode = "";
        ofstream outfile;

        if (powerState == POWER_STATE_STANDBY)
        {
            try {
                const device::SleepMode &mode = device::Host::getInstance().getPreferredSleepMode();
                sleepMode = mode.toString();
            } catch (...) {
                LOGWARN("Error getting PreferredStandbyMode");
                sleepMode = "";
            }

            LOGINFO("Output of preferredStandbyMode: '%s'", sleepMode.c_str());

            if (convert("DEEP_SLEEP", sleepMode))
            {
                errorCode = PowerManagerImplementation::_instance->SetDevicePowerState(keyCode, POWER_STATE_STANDBY_DEEP_SLEEP);
            }
            else
            {
                errorCode = PowerManagerImplementation::_instance->SetDevicePowerState(keyCode, (PowerState)powerState);
            }
            outfile.open(STANDBY_REASON_FILE, std::ios::out);
            if (outfile.is_open())
            {
                outfile << standbyReason;
                outfile.close();
            }
            else
            {
                LOGERR("Can't open file '%s' for write mode", STANDBY_REASON_FILE);
            }
        }
        else
        {
            errorCode = PowerManagerImplementation::_instance->SetDevicePowerState(keyCode, (PowerState)powerState);
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::GetTemperatureThresholds(float &high, float &critical ) const
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_GetTempThresholds_Param_t param = {};

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                IARM_BUS_PWRMGR_API_GetTemperatureThresholds,
                (void *)&param,
                sizeof(param));

        if (res == IARM_RESULT_SUCCESS) {
            high = param.tempHigh;
            critical = param.tempCritical;
            LOGINFO("Got current temperature thresholds: high: %f, critical: %f", high, critical);
            errorCode = Core::ERROR_NONE;
        } else {
            high = critical = 0;
            LOGWARN("[%s] IARM Call failed.", __FUNCTION__);
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::SetTemperatureThresholds(const float high, const float critical )
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        JsonObject args;
        IARM_Bus_PWRMgr_SetTempThresholds_Param_t param = {};

        param.tempHigh = high;
        param.tempCritical = critical;

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                IARM_BUS_PWRMGR_API_SetTemperatureThresholds,
                (void *)&param,
                sizeof(param));

        if (res == IARM_RESULT_SUCCESS) {
            LOGINFO("Set new temperature thresholds: high: %f, critical: %f", high, critical);
            errorCode = Core::ERROR_NONE;
        } else {
            LOGWARN("[%s] IARM Call failed.", __FUNCTION__);
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::GetOvertempGraceInterval(int& graceInterval ) const
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_GetOvertempGraceInterval_Param_t param = {};

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                IARM_BUS_PWRMGR_API_GetOvertempGraceInterval,
                (void *)&param,
                sizeof(param));

        if (res == IARM_RESULT_SUCCESS) {
            graceInterval = param.graceInterval;
            LOGINFO("Got current overtemparature grace inetrval: %d", graceInterval);
            errorCode = Core::ERROR_NONE;
        } else {
            graceInterval = 0;
            LOGWARN("[%s] IARM Call failed.", __FUNCTION__);
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::SetOvertempGraceInterval(const int graceInterval )
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_SetOvertempGraceInterval_Param_t param = {};
        param.graceInterval = graceInterval;

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                IARM_BUS_PWRMGR_API_SetOvertempGraceInterval,
                (void *)&param,
                sizeof(param));

        if (res == IARM_RESULT_SUCCESS) {
            LOGINFO("Set new overtemparature grace interval: %d", graceInterval);
            errorCode = Core::ERROR_NONE;
        } else {
            LOGWARN("[%s] IARM Call failed.", __FUNCTION__);
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::GetThermalState(float& temperature ) const
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
#ifdef ENABLE_THERMAL_PROTECTION
        IARM_Bus_PWRMgr_GetThermalState_Param_t param = {};

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                    IARM_BUS_PWRMGR_API_GetThermalState, (void *)&param, sizeof(param));

        if (res == IARM_RESULT_SUCCESS) {
                temperature = param.curTemperature;
                LOGINFO("Current core temperature is : %f ",temperature);
                errorCode = Core::ERROR_NONE;
        } else {
                LOGERR("[%s] IARM Call failed.", __FUNCTION__);
        }
#else
        temperature = -1;
        errorCode = Core::ERROR_GENERAL;
        LOGWARN("Thermal Protection disabled for this platform");
#endif
        return errorCode;
    }

    uint32_t PowerManagerImplementation::SetDeepSleepTimer(const int timeOut )
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_SetDeepSleepTimeOut_Param_t param = {};
        param.timeout = timeOut;
        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                                    IARM_BUS_PWRMGR_API_SetDeepSleepTimeOut, (void *)&param,
                                    sizeof(param));

        if (IARM_RESULT_SUCCESS == res)
        {
            errorCode = Core::ERROR_NONE;
        }
        return errorCode;
    }

    uint32_t PowerManagerImplementation::GetLastWakeupReason(WakeupReason &wakeupReason ) const
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        DeepSleep_WakeupReason_t deepSleepWakeupReason = DEEPSLEEP_WAKEUPREASON_IR;

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
			IARM_BUS_PWRMGR_API_GetLastWakeupReason, (void *)&deepSleepWakeupReason,
			sizeof(deepSleepWakeupReason));

        if (IARM_RESULT_SUCCESS == res)
        {
            wakeupReason = PowerManagerImplementation::_instance->ConvertToDeepSleepWakeupReason(deepSleepWakeupReason);
            errorCode = Core::ERROR_NONE;
        }
        return errorCode;
    }

    uint32_t PowerManagerImplementation::GetLastWakeupKeyCode(int &keycode ) const
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        DeepSleepMgr_WakeupKeyCode_Param_t param = {};

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                         IARM_BUS_PWRMGR_API_GetLastWakeupKeyCode, (void *)&param,
                         sizeof(param));
        if (IARM_RESULT_SUCCESS == res)
        {
            errorCode = Core::ERROR_NONE;
            keycode = param.keyCode;
        }

        LOGINFO("WakeupKeyCode : %d", keycode);

        return errorCode;
    }

    uint32_t PowerManagerImplementation::Reboot(const string &rebootRequestor, const string &rebootReasonCustom, const string &rebootReasonOther)
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        bool nfxResult = false;
        string requestor = "PowerManager";
        string customReason = "No custom reason provided";
        string otherReason = "No other reason supplied";
        string fname = "nrdPluginApp";

        nfxResult = Utils::killProcess(fname);
        if (true == nfxResult)
        {
            LOGINFO("PowerManager shutting down Netflix...");
            //give Netflix process some time to terminate gracefully.
            sleep(10);
        }
        else
        {
            LOGINFO("PowerManager unable to shutdown Netflix process. nfxResult = %ld", (long int)nfxResult);
        }

        if (!rebootRequestor.empty())
        {
            requestor = rebootRequestor;
        }

        if (!rebootReasonCustom.empty())
        {
            customReason = rebootReasonCustom;
            otherReason = customReason;
        }

        if (!rebootReasonOther.empty())
        {
            otherReason = rebootReasonOther;
        }

        IARM_Bus_PWRMgr_RebootParam_t rebootParam;
        strncpy(rebootParam.requestor, requestor.c_str(), sizeof(rebootParam.requestor));
        rebootParam.requestor[sizeof(rebootParam.requestor) - 1] = '\0';
        strncpy(rebootParam.reboot_reason_custom, customReason.c_str(), sizeof(rebootParam.reboot_reason_custom));
        rebootParam.reboot_reason_custom[sizeof(rebootParam.reboot_reason_custom) - 1] = '\0';
        strncpy(rebootParam.reboot_reason_other, otherReason.c_str(), sizeof(rebootParam.reboot_reason_other));
        rebootParam.reboot_reason_other[sizeof(rebootParam.reboot_reason_other) - 1] = '\0';
        LOGINFO("requestSystemReboot: requestor %s, custom reason: %s, other reason: %s", rebootParam.requestor, rebootParam.reboot_reason_custom,
                rebootParam.reboot_reason_other);

        IARM_Result_t iarmcallstatus = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                    IARM_BUS_PWRMGR_API_Reboot, &rebootParam, sizeof(rebootParam));
        if(IARM_RESULT_SUCCESS == iarmcallstatus)
        {
            errorCode = Core::ERROR_NONE;
        }
        else
        {
            LOGWARN("requestSystemReboot: IARM_BUS_PWRMGR_API_Reboot failed with code %d.", iarmcallstatus);
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::SetNetworkStandbyMode( const bool &standbyMode )
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_NetworkStandbyMode_Param_t param = {};

        param.bStandbyMode = standbyMode;
        LOGINFO("setNetworkStandbyMode called, with NwStandbyMode : %s",
                          (param.bStandbyMode)?("Enabled"):("Disabled"));
        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                                        IARM_BUS_PWRMGR_API_SetNetworkStandbyMode, (void *)&param,
                                        sizeof(param));

        if (IARM_RESULT_SUCCESS == res)
        {
            errorCode = Core::ERROR_NONE;
            m_networkStandbyModeValid = false;
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::GetNetworkStandbyMode(bool &standbyMode )
    {
        uint32_t errorCode = Core::ERROR_GENERAL;

        if (m_networkStandbyModeValid)
        {
            standbyMode = m_networkStandbyMode;
            errorCode = Core::ERROR_NONE;
            LOGINFO("Got cached NetworkStandbyMode: '%s'", m_networkStandbyMode ? "true" : "false");
        }
        else
        {
            IARM_Bus_PWRMgr_NetworkStandbyMode_Param_t param = {};
            IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                                   IARM_BUS_PWRMGR_API_GetNetworkStandbyMode, (void *)&param,
                                   sizeof(param));
            standbyMode = param.bStandbyMode;

            LOGINFO("getNetworkStandbyMode called, current NwStandbyMode is: %s",
                     standbyMode?("Enabled"):("Disabled"));

            if (IARM_RESULT_SUCCESS == res)
            {
                errorCode = Core::ERROR_NONE;
                m_networkStandbyMode = standbyMode;
                m_networkStandbyModeValid = true;
            }
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::SetWakeupSrcConfig(const int &powerMode, const int &srcType, int config)
    {
        uint32_t errorCode = Core::ERROR_GENERAL;

        LOGINFO(" Power State stored: %x srcType:%x  config :%x ",powerMode,srcType ,config);
        if(srcType) {
            IARM_Bus_PWRMgr_WakeupSrcConfig_Param_t param = {};
            param.pwrMode = powerMode >> 1;
            param.srcType = srcType>>1;
            param.config = config>>1;
            LOGINFO(" Power State stored: %x srcType:%x  config :%x ",powerMode,param.srcType ,param.config);

            if((config & (1 << WAKEUPSRC_WIFI)) || (config & (1<<WAKEUPSRC_LAN)))
            {
                m_networkStandbyModeValid = false;
            }
            IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                                   IARM_BUS_PWRMGR_API_SetWakeupSrcConfig, (void *)&param,
                                   sizeof(param));
            if (IARM_RESULT_SUCCESS == res) {
                errorCode = Core::ERROR_NONE;
            }
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::GetWakeupSrcConfig(int &powerMode, int &srcType, int &config) const
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_WakeupSrcConfig_Param_t param = {0,0,0};
        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                              IARM_BUS_PWRMGR_API_GetWakeupSrcConfig, (void *)&param,
                              sizeof(param));

        if (IARM_RESULT_SUCCESS == res)
        {
            LOGINFO(" %s: %d res:%d srcType :%x  config :%x ",__FUNCTION__,__LINE__,res,param.srcType,param.config);
            errorCode = Core::ERROR_NONE;

            srcType = param.srcType << 1;
            powerMode = param.pwrMode << 1;
            config = param.config << 1;
            LOGINFO(" %s: %d res:%d srcType :%x  config :%x ",__FUNCTION__,__LINE__,res,srcType,config);
        }
        return errorCode;
    }

    uint32_t PowerManagerImplementation::SetSystemMode(const SystemMode &currentMode, const SystemMode &newMode) const
    {
        uint32_t errorCode = Core::ERROR_GENERAL;
        IARM_Bus_CommonAPI_SysModeChange_Param_t modeParam;

        modeParam.oldMode = PowerManagerImplementation::_instance->ConvertToDaemonSystemMode(currentMode);
        modeParam.newMode = PowerManagerImplementation::_instance->ConvertToDaemonSystemMode(newMode);
LOGINFO("switched to mode '%d' to '%d'",currentMode, newMode);
        if (IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_DAEMON_NAME,
                "DaemonSysModeChange", &modeParam, sizeof(modeParam)))
        {
            LOGINFO("switched to mode '%d'", newMode);
            errorCode = Core::ERROR_NONE;
        }

        return errorCode;
    }

    uint32_t PowerManagerImplementation::GetPowerStateBeforeReboot(PowerState &powerStateBeforeReboot)
    {
        uint32_t errorCode = Core::ERROR_GENERAL;

        if (m_powerStateBeforeRebootValid)
        {
            powerStateBeforeReboot = m_powerStateBeforeReboot;

            errorCode = Core::ERROR_NONE;
            LOGINFO("Got cached powerStateBeforeReboot: '%u'", m_powerStateBeforeReboot);
        }
        else
        {
            IARM_Bus_PWRMgr_GetPowerStateBeforeReboot_Param_t param;
            IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                                       IARM_BUS_PWRMGR_API_GetPowerStateBeforeReboot, (void *)&param,
                                       sizeof(param));

            LOGWARN("getPowerStateBeforeReboot called, current powerStateBeforeReboot is: %s",
                         param.powerStateBeforeReboot);

            if (!strcmp("ON",param.powerStateBeforeReboot))
            {
                powerStateBeforeReboot = POWER_STATE_ON;
            }
            else if (!strcmp("OFF",param.powerStateBeforeReboot))
            {
                powerStateBeforeReboot = POWER_STATE_OFF;
            }
            else if (!strcmp("STANDBY",param.powerStateBeforeReboot))
            {
                powerStateBeforeReboot = POWER_STATE_STANDBY;
            }
            else if (!strcmp("DEEP_SLEEP",param.powerStateBeforeReboot))
            {
                powerStateBeforeReboot = POWER_STATE_STANDBY_DEEP_SLEEP;
            }
            else if (!strcmp("LIGHT_SLEEP",param.powerStateBeforeReboot))
            {
                powerStateBeforeReboot = POWER_STATE_STANDBY_LIGHT_SLEEP;
            }
            else
            {
                powerStateBeforeReboot = POWER_STATE_UNKNOWN;
            }

            if (IARM_RESULT_SUCCESS == res)
            {
                errorCode = Core::ERROR_NONE;
                m_powerStateBeforeReboot = powerStateBeforeReboot;
                m_powerStateBeforeRebootValid = true;
            }
        }

        return errorCode;
    }

    }
}

