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

#include "PowerManagerImplementation.h"

#include "UtilsIarm.h"
#include "UtilsLogging.h"

#include "rdk/iarmmgrs-hal/pwrMgr.h"

#define STANDBY_REASON_FILE "/opt/standbyReason.txt"
#define IARM_BUS_PWRMGR_API_SetDeepSleepTimeOut "SetDeepSleepTimeOut" /*!< Sets the timeout for deep sleep*/

using PreModeChangeTimer = WPEFramework::Plugin::PowerManagerImplementation::PreModeChangeTimer;

template <>
WPEFramework::Core::TimerType<PreModeChangeTimer> PreModeChangeTimer::timerThread(16 * 1024, "ACK TIMER");

int WPEFramework::Plugin::PowerManagerImplementation::PreModeChangeController::_nextTransactionId = 0;
uint32_t WPEFramework::Plugin::PowerManagerImplementation::_nextClientId = 0;

#ifndef POWER_MODE_PRECHANGE_TIMEOUT_SEC
#define POWER_MODE_PRECHANGE_TIMEOUT_SEC 3
#endif

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
    static void _iarmPowerEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len);

    SERVICE_REGISTRATION(PowerManagerImplementation, 1, 0);
    PowerManagerImplementation* PowerManagerImplementation::_instance = nullptr;
    PowerManagerImplementation::PowerManagerImplementation()
        : m_powerStateBeforeReboot(POWER_STATE_UNKNOWN)
        , m_networkStandbyMode(false)
        , m_networkStandbyModeValid(false)
        , m_powerStateBeforeRebootValid(false)
        , _modeChangeController(nullptr)
    {
        PowerManagerImplementation::_instance = this;

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
        if (Utils::IARM::init()) {
            IARM_Result_t res = IARM_RESULT_INVALID_PARAM;
            IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, _iarmPowerEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_REBOOTING, _iarmPowerEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED, _iarmPowerEventHandler));
            IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_DEEPSLEEP_TIMEOUT, _iarmPowerEventHandler));

#ifdef ENABLE_THERMAL_PROTECTION
            IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_THERMAL_MODECHANGED, _iarmPowerEventHandler));
#endif // ENABLE_THERMAL_PROTECTION
        }
    }

    void PowerManagerImplementation::DeinitializeIARM()
    {
        if (Utils::IARM::isConnected()) {
            IARM_Result_t res = IARM_RESULT_INVALID_PARAM;
            IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, _iarmPowerEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_REBOOTING, _iarmPowerEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED, _iarmPowerEventHandler));
            IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_DEEPSLEEP_TIMEOUT, _iarmPowerEventHandler));

#ifdef ENABLE_THERMAL_PROTECTION
            IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_THERMAL_MODECHANGED, _iarmPowerEventHandler));
#endif // ENABLE_THERMAL_PROTECTION
        }
    }
#endif /* defined(USE_IARMBUS) || defined(USE_IARM_BUS) */

    void PowerManagerImplementation::dispatchPowerModeChangedEvent(const PowerState& currentState, const PowerState& newState)
    {
        auto index = _modeChangedNotifications.begin();
        while (index != _modeChangedNotifications.end()) {
            (*index)->OnPowerModeChanged(currentState, newState);
            ++index;
        }
    }

    void PowerManagerImplementation::dispatchDeepSleepTimeoutEvent(const uint32_t& timeout)
    {
        std::list<Exchange::IPowerManager::IDeepSleepTimeoutNotification*>::const_iterator index(_deepSleepTimeoutNotifications.begin());
        while (index != _deepSleepTimeoutNotifications.end()) {
            (*index)->OnDeepSleepTimeout(timeout);
            ++index;
        }
    }

    void PowerManagerImplementation::dispatchRebootBeginEvent(const string& rebootReasonCustom, const string& rebootReasonOther, const string& rebootRequestor)
    {
        std::list<Exchange::IPowerManager::IRebootNotification*>::const_iterator index(_rebootNotifications.begin());
        while (index != _rebootNotifications.end()) {
            (*index)->OnRebootBegin(rebootReasonCustom, rebootReasonOther, rebootRequestor);
            ++index;
        }
    }

    void PowerManagerImplementation::dispatchThermalModeChangedEvent(const ThermalTemperature& currentThermalLevel, const ThermalTemperature& newThermalLevel, const float& currentTemperature)
    {
        std::list<Exchange::IPowerManager::IThermalModeChangedNotification*>::const_iterator index(_thermalModeChangedNotifications.begin());
        while (index != _thermalModeChangedNotifications.end()) {
            (*index)->OnThermalModeChanged(currentThermalLevel, newThermalLevel, currentTemperature);
            ++index;
        }
    }

    void PowerManagerImplementation::dispatchNetworkStandbyModeChangedEvent(const bool& enabled)
    {
        std::list<Exchange::IPowerManager::INetworkStandbyModeChangedNotification*>::const_iterator index(_networkStandbyModeChangedNotifications.begin());
        while (index != _networkStandbyModeChangedNotifications.end()) {
            (*index)->OnNetworkStandbyModeChanged(enabled);
            ++index;
        }
    }

    void PowerManagerImplementation::Dispatch(Event event, ParamsType params)
    {
        _adminLock.Lock();

        switch (event) {
        case PWRMGR_EVENT_POWERMODE_CHANGED:
            if (const auto* pairValue = boost::get<std::pair<PowerState, PowerState>>(&params)) {
                PowerState currentState = pairValue->first, newState = pairValue->second;
                dispatchPowerModeChangedEvent(currentState, newState);
            }
            break;

        case PWRMGR_EVENT_DEEPSLEEP_TIMEOUT:
            if (const uint32_t* value = boost::get<uint32_t>(&params)) {
                dispatchDeepSleepTimeoutEvent(*value);
            }
            break;

        case PWRMGR_EVENT_REBOOTING:
            if (const auto* tupleValue = boost::get<std::tuple<std::string, std::string, std::string>>(&params)) {
                string rebootReasonCustom = std::get<0>(*tupleValue), rebootReasonOther = std::get<1>(*tupleValue), rebootRequestor = std::get<2>(*tupleValue);
                dispatchRebootBeginEvent(rebootReasonCustom, rebootReasonOther, rebootRequestor);
            }
            break;

        case PWRMGR_EVENT_THERMAL_MODECHANGED:
            if (const auto* thermData = boost::get<std::tuple<ThermalTemperature, ThermalTemperature, float>>(&params)) {
                ThermalTemperature currentThermalLevel = std::get<0>(*thermData), newThermalLevel = std::get<1>(*thermData);
                float currentTemperature = std::get<2>(*thermData);
                dispatchThermalModeChangedEvent(currentThermalLevel, newThermalLevel, currentTemperature);
            }
            break;

        case PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED:
            if (const bool* boolValue = boost::get<bool>(&params)) {
                dispatchNetworkStandbyModeChangedEvent(*boolValue);
            }
            break;

        default:
            LOGERR("Unhandled event: %d", event);
        } // switch (event)

        _adminLock.Unlock();
    }

    static void _iarmPowerEventHandler(const char* owner, IARM_EventId_t eventId, void* data, size_t len)
    {
        switch (eventId) {
        case IARM_BUS_PWRMGR_EVENT_MODECHANGED: {
            IARM_Bus_PWRMgr_EventData_t* eventData = (IARM_Bus_PWRMgr_EventData_t*)data;
            auto pairParam = std::make_pair(PowerManagerImplementation::_instance->ConvertToPwrMgrPowerState(eventData->data.state.curState),
                PowerManagerImplementation::_instance->ConvertToPwrMgrPowerState(eventData->data.state.newState));

            LOGINFO("IARM Event triggered for PowerStateChange. Old State %u, New State: %u",
                eventData->data.state.curState, eventData->data.state.newState);

            Core::IWorkerPool::Instance().Submit(PowerManagerImplementation::Job::Create(PowerManagerImplementation::_instance,
                PowerManagerImplementation::PWRMGR_EVENT_POWERMODE_CHANGED,
                pairParam));
        } break;

        case IARM_BUS_PWRMGR_EVENT_REBOOTING: {
            IARM_Bus_PWRMgr_RebootParam_t* eventData = (IARM_Bus_PWRMgr_RebootParam_t*)data;
            auto tupleParam = std::make_tuple(eventData->reboot_reason_custom,
                eventData->reboot_reason_other,
                eventData->requestor);

            LOGINFO("IARM Event triggered for reboot. reboot_reason_custom: %s, requestor: %s, reboot_reason_other: %s",
                eventData->reboot_reason_custom, eventData->requestor, eventData->reboot_reason_other);

            Core::IWorkerPool::Instance().Submit(PowerManagerImplementation::Job::Create(PowerManagerImplementation::_instance,
                PowerManagerImplementation::PWRMGR_EVENT_REBOOTING,
                tupleParam));
        } break;

        case IARM_BUS_PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED: {
            IARM_Bus_PWRMgr_EventData_t* eventData = (IARM_Bus_PWRMgr_EventData_t*)data;

            LOGINFO("IARM Event triggered for NetworkStandbyMode. NetworkStandbyMode: %u", eventData->data.bNetworkStandbyMode);

            Core::IWorkerPool::Instance().Submit(PowerManagerImplementation::Job::Create(PowerManagerImplementation::_instance,
                PowerManagerImplementation::PWRMGR_EVENT_NETWORK_STANDBYMODECHANGED,
                eventData->data.bNetworkStandbyMode));
        } break;

        case IARM_BUS_PWRMGR_EVENT_THERMAL_MODECHANGED: {
            IARM_Bus_PWRMgr_EventData_t* eventData = (IARM_Bus_PWRMgr_EventData_t*)data;
            auto tupleParam = std::make_tuple(PowerManagerImplementation::_instance->ConvertToThermalState(eventData->data.therm.curLevel),
                PowerManagerImplementation::_instance->ConvertToThermalState(eventData->data.therm.newLevel),
                (int)eventData->data.therm.curTemperature);

            Core::IWorkerPool::Instance().Submit(PowerManagerImplementation::Job::Create(PowerManagerImplementation::_instance,
                PowerManagerImplementation::PWRMGR_EVENT_THERMAL_MODECHANGED,
                tupleParam));

            LOGINFO("THERMAL_MODECHANGED event received, curLevel: %u, newLevel: %u, curTemperature: %f",
                eventData->data.therm.curLevel, eventData->data.therm.newLevel, eventData->data.therm.curTemperature);
        } break;

        case IARM_BUS_PWRMGR_EVENT_DEEPSLEEP_TIMEOUT: {
            IARM_BUS_PWRMgr_DeepSleepTimeout_EventData_t* eventData = (IARM_BUS_PWRMgr_DeepSleepTimeout_EventData_t*)data;

            Core::IWorkerPool::Instance().Submit(PowerManagerImplementation::Job::Create(PowerManagerImplementation::_instance,
                PowerManagerImplementation::PWRMGR_EVENT_DEEPSLEEP_TIMEOUT,
                eventData->timeout));

            LOGINFO("IARM Event triggered for deep sleep timeout: %u", eventData->timeout);
        } break;

        default:
            LOGWARN("Unhandled IARM event: %d", eventId);
        } // switch (eventId)
    }

    template <typename T>
    Core::hresult PowerManagerImplementation::Register(std::list<T*>& list, T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);
        _adminLock.Lock();

        // Make sure we can't register the same notification callback multiple times
        if (std::find(list.begin(), list.end(), notification) == list.end()) {
            list.push_back(notification);
            notification->AddRef();
            status = Core::ERROR_NONE;
        }

        _adminLock.Unlock();
        return status;
    }

    template <typename T>
    Core::hresult PowerManagerImplementation::Unregister(std::list<T*>& list, T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);
        _adminLock.Lock();

        // Make sure we can't unregister the same notification callback multiple times
        auto itr = std::find(list.begin(), list.end(), notification);
        if (itr != list.end()) {
            (*itr)->Release();
            list.erase(itr);
            status = Core::ERROR_NONE;
        }

        _adminLock.Unlock();
        return status;
    }

    Core::hresult PowerManagerImplementation::Register(Exchange::IPowerManager::IRebootNotification* notification)
    {
        Core::hresult errorCode = Register(_rebootNotifications, notification);
        LOGINFO("IRebootNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Unregister(Exchange::IPowerManager::IRebootNotification* notification)
    {
        Core::hresult errorCode = Unregister(_rebootNotifications, notification);
        LOGINFO("IRebootNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Register(Exchange::IPowerManager::IModePreChangeNotification* notification)
    {
        Core::hresult errorCode = Register(_preModeChangeNotifications, notification);
        LOGINFO("IModePreChangeNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Unregister(Exchange::IPowerManager::IModePreChangeNotification* notification)
    {
        Core::hresult errorCode = Unregister(_preModeChangeNotifications, notification);
        LOGINFO("IModePreChangeNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Register(Exchange::IPowerManager::IModeChangedNotification* notification)
    {
        Core::hresult errorCode = Register(_modeChangedNotifications, notification);
        LOGINFO("IModeChangedNotification %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Unregister(Exchange::IPowerManager::IModeChangedNotification* notification)
    {
        Core::hresult errorCode = Unregister(_modeChangedNotifications, notification);
        LOGINFO("IModeChangedNotification %p, errorcode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Register(Exchange::IPowerManager::IDeepSleepTimeoutNotification* notification)
    {
        Core::hresult errorCode = Register(_deepSleepTimeoutNotifications, notification);
        LOGINFO("IDeepSleepTimeoutNotification %p, errorcode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Unregister(Exchange::IPowerManager::IDeepSleepTimeoutNotification* notification)
    {
        Core::hresult errorCode = Unregister(_deepSleepTimeoutNotifications, notification);
        LOGINFO("IDeepSleepTimeoutNotification %p, errorcode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Register(Exchange::IPowerManager::INetworkStandbyModeChangedNotification* notification)
    {
        Core::hresult errorCode = Register(_networkStandbyModeChangedNotifications, notification);
        LOGINFO("INetworkStandbyModeChangedNotification %p, errorcode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Unregister(Exchange::IPowerManager::INetworkStandbyModeChangedNotification* notification)
    {
        Core::hresult errorCode = Unregister(_networkStandbyModeChangedNotifications, notification);
        LOGINFO("INetworkStandbyModeChangedNotification %p, errorcode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Register(Exchange::IPowerManager::IThermalModeChangedNotification* notification)
    {
        Core::hresult errorCode = Register(_thermalModeChangedNotifications, notification);
        LOGINFO("IThermalModeChangedNotification %p, errorcode: %u", notification, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Unregister(Exchange::IPowerManager::IThermalModeChangedNotification* notification)
    {
        Core::hresult errorCode = Unregister(_thermalModeChangedNotifications, notification);
        LOGINFO("IThermalModeChangedNotification %p, errorcode: %u", notification, errorCode);
        return errorCode;
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
            LOGERR("Unknown power state: %d, defaulting to IARM_BUS_PWRMGR_POWERSTATE_OFF", state);
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
            LOGERR("Unknown IARM power state: %d, defaulting to POWER_STATE_UNKNOWN", state);
            break;
        }

        return powerState;
    }

    WakeupReason PowerManagerImplementation::ConvertToDeepSleepWakeupReason(DeepSleep_WakeupReason_t deepSleepWakeupReason)
    {
        WakeupReason wakeupReason;

        switch (deepSleepWakeupReason) {
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
            LOGERR("Unknown deep sleep wakeup reason: %d, defaulting to WAKEUP_REASON_UNKNOWN", deepSleepWakeupReason);
            break;
        }

        return wakeupReason;
    }

    ThermalTemperature PowerManagerImplementation::ConvertToThermalState(IARM_Bus_PWRMgr_ThermalState_t thermalState)
    {
        ThermalTemperature pwrMgrThermState;

        switch (thermalState) {
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
            LOGERR("Unknown thermal state: %d, defaulting to THERMAL_TEMPERATURE_UNKNOWN", thermalState);
            break;
        }

        return pwrMgrThermState;
    }

    IARM_Bus_Daemon_SysMode_t PowerManagerImplementation::ConvertToDaemonSystemMode(SystemMode sysMode)
    {
        IARM_Bus_Daemon_SysMode_t systemMode = IARM_BUS_SYS_MODE_NORMAL;

        switch (sysMode) {
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
            LOGERR("Unknown system mode: %d, defaulting to IARM_BUS_SYS_MODE_NORMAL", sysMode);
            break;
        }

        return systemMode;
    }

    Core::hresult PowerManagerImplementation::GetPowerState(PowerState& currentState, PowerState& prevState) const
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_GetPowerState_Param_t param = {};

        LOGINFO("impl called for GetPowerState()");
        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_API_GetPowerState,
            (void*)&param, sizeof(param));

        if (IARM_RESULT_SUCCESS == res) {
            currentState = PowerManagerImplementation::_instance->ConvertToPwrMgrPowerState(param.curState);
            prevState = PowerManagerImplementation::_instance->ConvertToPwrMgrPowerState(param.prevState);
            errorCode = Core::ERROR_NONE;
        }

        LOGINFO("getPowerState called, currentState : %u, prevState : %u", currentState, prevState);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetDevicePowerState(const int& keyCode, PowerState powerState)
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_SetPowerState_Param_t param = {};

        param.newState = PowerManagerImplementation::_instance->ConvertToIarmBusPowerState(powerState);

        if (POWER_STATE_UNKNOWN != powerState) {
            param.keyCode = keyCode;
            IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_API_SetPowerState,
                (void*)&param, sizeof(param));

            if (IARM_RESULT_SUCCESS == res) {
                errorCode = Core::ERROR_NONE;
            }
        } else {
            LOGWARN("Invalid power state is received %u", powerState);
        }

        LOGINFO("SetDevicePowerState keyCode: %d, powerState: %u, errorcode: %u", keyCode, powerState, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetPowerState(const int keyCode, const PowerState powerState, const string& standbyReason)
    {
        PowerState currentState = POWER_STATE_UNKNOWN;
        PowerState prevState = POWER_STATE_UNKNOWN;

        Core::hresult errorCode = GetPowerState(currentState, prevState);

        if (Core::ERROR_NONE != errorCode) {
            LOGERR("Failed to get current power state, errorCode: %d", errorCode);
            return errorCode;
        }

        if (currentState != powerState) {

            _adminLock.Lock();

            if (_modeChangeController) {
                LOGWARN("Power state change is already in progress, cancel old request");
                _modeChangeController.reset();
            }

            _modeChangeController = std::unique_ptr<PreModeChangeController>(new PreModeChangeController());
            const int transactionId = _modeChangeController->TransactionId();

            for (const auto& client : _modeChangeClients) {
                _modeChangeController->AckAwait(client.first);
            }

            _adminLock.Unlock();

            // dispatch pre power mode change notifications
            submitPowerModePreChangeEvent(currentState, powerState, transactionId);

            // like in `Job` class we avoid impl destruction before handler is invoked
            this->AddRef();

            _modeChangeController->Schedule(POWER_MODE_PRECHANGE_TIMEOUT_SEC * 1000,
                [this, keyCode, powerState](bool /*isTimedout*/) mutable {
                    PowerModePreChangeCompletionHandler(keyCode, powerState);
                });
        } else {
            LOGINFO("Requested power state is same as current power state, no action required");
        }

        LOGINFO("SetPowerState keyCode: %d, currentState: %u, powerState: %u, errorCode: %d",
                keyCode, currentState, powerState, Core::ERROR_NONE);

        return Core::ERROR_NONE;
    }

    void PowerManagerImplementation::submitPowerModePreChangeEvent(const PowerState currentState, const PowerState newState, const int transactionId)
    {
        for (auto& notification : _preModeChangeNotifications) {
            Core::IWorkerPool::Instance().Submit(
                PowerManagerImplementation::LambdaJob::Create(this,
                    [notification, currentState, newState, transactionId]() {
                        notification->OnPowerModePreChange(currentState, newState, transactionId, POWER_MODE_PRECHANGE_TIMEOUT_SEC);
                    }));
        }

        LOGINFO("currentState : %u, newState : %u, transactionId : %d", currentState, newState, transactionId);
    }

    Core::hresult PowerManagerImplementation::GetTemperatureThresholds(float& high, float& critical) const
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_GetTempThresholds_Param_t param = {};

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_GetTemperatureThresholds,
            (void*)&param,
            sizeof(param));

        if (IARM_RESULT_SUCCESS == res) {
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

    Core::hresult PowerManagerImplementation::SetTemperatureThresholds(const float high, const float critical)
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        JsonObject args;
        IARM_Bus_PWRMgr_SetTempThresholds_Param_t param = {};

        param.tempHigh = high;
        param.tempCritical = critical;

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_SetTemperatureThresholds,
            (void*)&param,
            sizeof(param));

        if (IARM_RESULT_SUCCESS == res) {
            LOGINFO("Set new temperature thresholds: high: %f, critical: %f", high, critical);
            errorCode = Core::ERROR_NONE;
        } else {
            LOGWARN("[%s] IARM Call failed.", __FUNCTION__);
        }

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetOvertempGraceInterval(int& graceInterval) const
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_GetOvertempGraceInterval_Param_t param = {};

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_GetOvertempGraceInterval,
            (void*)&param,
            sizeof(param));

        if (IARM_RESULT_SUCCESS == res) {
            graceInterval = param.graceInterval;
            LOGINFO("Got current overtemparature grace inetrval: %d", graceInterval);
            errorCode = Core::ERROR_NONE;
        } else {
            graceInterval = 0;
            LOGWARN("[%s] IARM Call failed.", __FUNCTION__);
        }

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetOvertempGraceInterval(const int graceInterval)
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_SetOvertempGraceInterval_Param_t param = {};
        param.graceInterval = graceInterval;

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_SetOvertempGraceInterval,
            (void*)&param,
            sizeof(param));

        if (IARM_RESULT_SUCCESS == res) {
            LOGINFO("Set new overtemparature grace interval: %d", graceInterval);
            errorCode = Core::ERROR_NONE;
        } else {
            LOGWARN("[%s] IARM Call failed.", __FUNCTION__);
        }

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetThermalState(float& temperature) const
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
#ifdef ENABLE_THERMAL_PROTECTION
        IARM_Bus_PWRMgr_GetThermalState_Param_t param = {};

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_GetThermalState, (void*)&param, sizeof(param));

        if (IARM_RESULT_SUCCESS == res) {
            temperature = param.curTemperature;
            LOGINFO("Current core temperature is : %f ", temperature);
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

    Core::hresult PowerManagerImplementation::SetDeepSleepTimer(const int timeOut)
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_SetDeepSleepTimeOut_Param_t param = {};
        param.timeout = timeOut;
        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_SetDeepSleepTimeOut, (void*)&param,
            sizeof(param));

        if (IARM_RESULT_SUCCESS == res) {
            errorCode = Core::ERROR_NONE;
        }
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetLastWakeupReason(WakeupReason& wakeupReason) const
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        DeepSleep_WakeupReason_t deepSleepWakeupReason = DEEPSLEEP_WAKEUPREASON_IR;

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_GetLastWakeupReason, (void*)&deepSleepWakeupReason,
            sizeof(deepSleepWakeupReason));

        if (IARM_RESULT_SUCCESS == res) {
            wakeupReason = PowerManagerImplementation::_instance->ConvertToDeepSleepWakeupReason(deepSleepWakeupReason);
            errorCode = Core::ERROR_NONE;
        }
        LOGINFO("WakeupReason: %d, errorCode: %u", wakeupReason, errorCode);
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetLastWakeupKeyCode(int& keycode) const
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        DeepSleepMgr_WakeupKeyCode_Param_t param = {};

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_GetLastWakeupKeyCode, (void*)&param,
            sizeof(param));
        if (IARM_RESULT_SUCCESS == res) {
            errorCode = Core::ERROR_NONE;
            keycode = param.keyCode;
        }

        LOGINFO("WakeupKeyCode : %d, errorcode: %u", keycode, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Reboot(const string& rebootRequestor, const string& rebootReasonCustom, const string& rebootReasonOther)
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        string requestor = "PowerManager";
        string customReason = "No custom reason provided";
        string otherReason = "No other reason supplied";

        if (!rebootRequestor.empty()) {
            requestor = rebootRequestor;
        }

        if (!rebootReasonCustom.empty()) {
            customReason = rebootReasonCustom;
            otherReason = customReason;
        }

        if (!rebootReasonOther.empty()) {
            otherReason = rebootReasonOther;
        }

        IARM_Bus_PWRMgr_RebootParam_t rebootParam;

        strncpy(rebootParam.requestor, requestor.c_str(), sizeof(rebootParam.requestor));
        rebootParam.requestor[sizeof(rebootParam.requestor) - 1] = '\0';

        strncpy(rebootParam.reboot_reason_custom, customReason.c_str(), sizeof(rebootParam.reboot_reason_custom));
        rebootParam.reboot_reason_custom[sizeof(rebootParam.reboot_reason_custom) - 1] = '\0';

        strncpy(rebootParam.reboot_reason_other, otherReason.c_str(), sizeof(rebootParam.reboot_reason_other));
        rebootParam.reboot_reason_other[sizeof(rebootParam.reboot_reason_other) - 1] = '\0';

        IARM_Result_t iarmcallstatus = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_Reboot, &rebootParam, sizeof(rebootParam));

        if (IARM_RESULT_SUCCESS == iarmcallstatus) {
            errorCode = Core::ERROR_NONE;
        }

        LOGINFO("requestor %s, custom reason: %s, other reason: %s, iarmstatus: %d, errorcode: %u", rebootParam.requestor, rebootParam.reboot_reason_custom,
            rebootParam.reboot_reason_other, iarmcallstatus, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetNetworkStandbyMode(const bool standbyMode)
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_NetworkStandbyMode_Param_t param = {};

        param.bStandbyMode = standbyMode;

        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_SetNetworkStandbyMode, (void*)&param,
            sizeof(param));

        if (IARM_RESULT_SUCCESS == res) {
            errorCode = Core::ERROR_NONE;
            m_networkStandbyModeValid = false;
        }

        LOGINFO("standbyMode : %s, errorcode: %u",
            (param.bStandbyMode) ? ("Enabled") : ("Disabled"), errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetNetworkStandbyMode(bool& standbyMode)
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;

        if (m_networkStandbyModeValid) {
            standbyMode = m_networkStandbyMode;
            errorCode = Core::ERROR_NONE;
            LOGINFO("Got cached NetworkStandbyMode: '%s'", m_networkStandbyMode ? "true" : "false");
        } else {
            IARM_Bus_PWRMgr_NetworkStandbyMode_Param_t param = {};
            IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                IARM_BUS_PWRMGR_API_GetNetworkStandbyMode, (void*)&param,
                sizeof(param));
            standbyMode = param.bStandbyMode;

            LOGINFO("current NwStandbyMode is: %s, res: %d",
                (standbyMode ? ("Enabled") : ("Disabled")), res);

            if (IARM_RESULT_SUCCESS == res) {
                errorCode = Core::ERROR_NONE;
                m_networkStandbyMode = standbyMode;
                m_networkStandbyModeValid = true;
            }
        }

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetWakeupSrcConfig(const int powerMode, const int srcType, int config)
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;

        LOGINFO(" Power State stored: %x srcType:%x  config :%x ", powerMode, srcType, config);
        if (srcType) {
            IARM_Bus_PWRMgr_WakeupSrcConfig_Param_t param = {};
            param.pwrMode = powerMode >> 1;
            param.srcType = srcType >> 1;
            param.config = config >> 1;

            if ((config & (1 << WAKEUPSRC_WIFI)) || (config & (1 << WAKEUPSRC_LAN))) {
                m_networkStandbyModeValid = false;
            }
            IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                IARM_BUS_PWRMGR_API_SetWakeupSrcConfig, (void*)&param,
                sizeof(param));
            if (IARM_RESULT_SUCCESS == res) {
                errorCode = Core::ERROR_NONE;
            }

            LOGINFO(" Power State stored: %x, srcType: %x,  config: %x, IARM status: %d", powerMode, param.srcType, param.config, res);
        }

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetWakeupSrcConfig(int& powerMode, int& srcType, int& config) const
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        IARM_Bus_PWRMgr_WakeupSrcConfig_Param_t param = { 0, 0, 0 };
        IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
            IARM_BUS_PWRMGR_API_GetWakeupSrcConfig, (void*)&param,
            sizeof(param));

        if (IARM_RESULT_SUCCESS == res) {
            LOGINFO("res:%d srcType :%x  config :%x ", res, param.srcType, param.config);
            errorCode = Core::ERROR_NONE;

            srcType = param.srcType << 1;
            powerMode = param.pwrMode << 1;
            config = param.config << 1;
            LOGINFO("res:%d srcType :%x  config :%x ", res, srcType, config);
        }
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetSystemMode(const SystemMode currentMode, const SystemMode newMode) const
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
        IARM_Bus_CommonAPI_SysModeChange_Param_t modeParam;

        modeParam.oldMode = PowerManagerImplementation::_instance->ConvertToDaemonSystemMode(currentMode);
        modeParam.newMode = PowerManagerImplementation::_instance->ConvertToDaemonSystemMode(newMode);

        LOGINFO("switched to mode '%d' to '%d'", currentMode, newMode);

        if (IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_DAEMON_NAME, "DaemonSysModeChange", &modeParam, sizeof(modeParam))) {
            errorCode = Core::ERROR_NONE;
        }

        LOGINFO("switched to mode '%d', errorcode: %u", newMode, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetPowerStateBeforeReboot(PowerState& powerStateBeforeReboot)
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;

        if (m_powerStateBeforeRebootValid) {
            powerStateBeforeReboot = m_powerStateBeforeReboot;

            errorCode = Core::ERROR_NONE;
            LOGINFO("Got cached powerStateBeforeReboot: '%u'", m_powerStateBeforeReboot);
        } else {
            IARM_Bus_PWRMgr_GetPowerStateBeforeReboot_Param_t param;
            IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME,
                IARM_BUS_PWRMGR_API_GetPowerStateBeforeReboot, (void*)&param,
                sizeof(param));

            LOGWARN("current powerStateBeforeReboot is: %s IARM status: %d",
                param.powerStateBeforeReboot, res);

            if (!strcmp("ON", param.powerStateBeforeReboot)) {
                powerStateBeforeReboot = POWER_STATE_ON;
            } else if (!strcmp("OFF", param.powerStateBeforeReboot)) {
                powerStateBeforeReboot = POWER_STATE_OFF;
            } else if (!strcmp("STANDBY", param.powerStateBeforeReboot)) {
                powerStateBeforeReboot = POWER_STATE_STANDBY;
            } else if (!strcmp("DEEP_SLEEP", param.powerStateBeforeReboot)) {
                powerStateBeforeReboot = POWER_STATE_STANDBY_DEEP_SLEEP;
            } else if (!strcmp("LIGHT_SLEEP", param.powerStateBeforeReboot)) {
                powerStateBeforeReboot = POWER_STATE_STANDBY_LIGHT_SLEEP;
            } else {
                powerStateBeforeReboot = POWER_STATE_UNKNOWN;
            }

            if (IARM_RESULT_SUCCESS == res) {
                errorCode = Core::ERROR_NONE;
                m_powerStateBeforeReboot = powerStateBeforeReboot;
                m_powerStateBeforeRebootValid = true;
            }
        }

        return errorCode;
    }

    void PowerManagerImplementation::PowerModePreChangeCompletionHandler(const int keyCode, PowerState powerState)
    {
        LOGINFO("PowerModePreChangeCompletionHandler triggered keyCode: %d, powerState: %u", keyCode, powerState);

        SetDevicePowerState(keyCode, powerState);

        // After moving power HAL to plugin, we might need context switching
        // Core::IWorkerPool::Instance().Submit(
        //     PowerManagerImplementation::LambdaJob::Create(this,
        //         [this, keyCode, powerState]() {
        //             SetDevicePowerState(keyCode, powerState);
        //         }));

        // release the refCount taken in SetPowerState => _modeChangeController->Schedule
        this->Release();

        // release controller as mode change is complete now
        _modeChangeController.reset();
    }

    Core::hresult PowerManagerImplementation::PowerModePreChangeComplete(const uint32_t clientId, const int transactionId)
    {
        Core::hresult errorCode = Core::ERROR_INVALID_PARAMETER;

        _adminLock.Lock();

        if (_modeChangeController) {
            errorCode = _modeChangeController->Ack(clientId, transactionId);
        }

        _adminLock.Unlock();

        LOGINFO("clientId: %u, transactionId: %d, errorcode: %u", clientId, transactionId, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::DelayPowerModeChangeBy(const uint32_t clientId, const int transactionId, const int delayPeriod)
    {
        Core::hresult errorCode = Core::ERROR_INVALID_PARAMETER;

        _adminLock.Lock();

        if (_modeChangeController) {
            errorCode = _modeChangeController->Reschedule(clientId, transactionId, delayPeriod * 1000);
        }

        _adminLock.Unlock();

        LOGINFO("DelayPowerModeChangeBy clientId: %u, transactionId: %d, delayPeriod: %d, errorcode: %u", clientId, transactionId, delayPeriod, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::AddPowerModePreChangeClient(const string& clientName, uint32_t& clientId)
    {
        if (clientName.empty()) {
            LOGERR("AddPowerModePreChangeClient called with empty clientName");
            return Core::ERROR_INVALID_PARAMETER;
        }

        _adminLock.Lock();

        auto it = std::find_if(_modeChangeClients.cbegin(), _modeChangeClients.cend(),
            [&clientName](const std::pair<uint32_t, string>& client) {
                return client.second == clientName;
            });

        if (it == _modeChangeClients.end()) {
            clientId = ++_nextClientId;
            _modeChangeClients[clientId] = clientName;
        } else {
            // client is already registered, return the clientId
            clientId = it->first;
        }

        _adminLock.Unlock();

        LOGINFO("client: %s, clientId: %u", clientName.c_str(), clientId);

        for (auto& clients : _modeChangeClients) {
            LOGINFO("Registered client: %s, clientId: %u", clients.second.c_str(), clients.first);
        }

        return Core::ERROR_NONE;
    }

    Core::hresult PowerManagerImplementation::RemovePowerModePreChangeClient(const uint32_t clientId)
    {
        Core::hresult errorCode = Core::ERROR_INVALID_PARAMETER;
        std::string clientName;

        _adminLock.Lock();

        auto it = _modeChangeClients.find(clientId);

        if (it != _modeChangeClients.end()) {
            clientName = it->second;
            _modeChangeClients.erase(it);

            // self-ack if called while power mode change is in progress
            if (_modeChangeController) {
                _modeChangeController->Ack(clientId);
            }
            errorCode = Core::ERROR_NONE;
        }

        _adminLock.Unlock();

        LOGINFO("client: %s, clientId: %u, errorcode: %u", clientName.c_str(), clientId, errorCode);

        return errorCode;
    }
}
}
