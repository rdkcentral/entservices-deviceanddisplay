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
#include "PowerUtils.h"

#include <core/Portability.h>
#include <interfaces/IPowerManager.h>

#define STANDBY_REASON_FILE "/opt/standbyReason.txt"
#define IARM_BUS_PWRMGR_API_SetDeepSleepTimeOut "SetDeepSleepTimeOut" /*!< Sets the timeout for deep sleep*/

using PreModeChangeTimer = WPEFramework::Plugin::PowerManagerImplementation::PreModeChangeTimer;
using util = PowerUtils;

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

    SERVICE_REGISTRATION(PowerManagerImplementation, 1, 0);
    PowerManagerImplementation* PowerManagerImplementation::_instance = nullptr;

    PowerManagerImplementation::PowerManagerImplementation()
        : m_powerStateBeforeReboot(POWER_STATE_UNKNOWN)
        , m_networkStandbyMode(false)
        , m_networkStandbyModeValid(false)
        , m_powerStateBeforeRebootValid(false)
        , _modeChangeController(nullptr)
        , _deepSleepController(DeepSleepController::Create(*this))
        , _powerController(PowerController::Create(_deepSleepController))
        , _thermalController(ThermalController::Create(*this))
    {
        PowerManagerImplementation::_instance = this;
        Utils::IARM::init();
    }

    PowerManagerImplementation::~PowerManagerImplementation()
    {
    }

    void PowerManagerImplementation::dispatchPowerModeChangedEvent(const PowerState& prevState, const PowerState& newState)
    {
        auto index = _modeChangedNotifications.begin();
        while (index != _modeChangedNotifications.end()) {
            (*index)->OnPowerModeChanged(prevState, newState);
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

    template <typename T>
    Core::hresult PowerManagerImplementation::Register(std::list<T*>& list, T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);
        _callbackLock.Lock();

        // Make sure we can't register the same notification callback multiple times
        if (std::find(list.begin(), list.end(), notification) == list.end()) {
            list.push_back(notification);
            notification->AddRef();
            status = Core::ERROR_NONE;
        }

        _callbackLock.Unlock();
        return status;
    }

    template <typename T>
    Core::hresult PowerManagerImplementation::Unregister(std::list<T*>& list, T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);
        _callbackLock.Lock();

        // Make sure we can't unregister the same notification callback multiple times
        auto itr = std::find(list.begin(), list.end(), notification);
        if (itr != list.end()) {
            (*itr)->Release();
            list.erase(itr);
            status = Core::ERROR_NONE;
        }

        _callbackLock.Unlock();
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

    Core::hresult PowerManagerImplementation::GetPowerState(PowerState& currentState, PowerState& prevState) const
    {
        _apiLock.Lock();

        uint32_t errorCode = _powerController.GetPowerState(currentState, prevState);

        _apiLock.Unlock();

        LOGINFO("currentState : %s, prevState : %s, errorCode = %d", util::str(currentState), util::str(prevState), errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::setDevicePowerState(const int& keyCode, PowerState prevState, PowerState newState, const std::string& reason)
    {
        uint32_t errorCode = _powerController.SetPowerState(keyCode, newState, reason);

        if (Core::ERROR_NONE != errorCode) {
            LOGERR("Failed to set power state, errorCode: %d", errorCode);
            return errorCode;
        }

        // We don't do a thread switching here, as it may move device to deep sleep mode
        // even before client receiving the event
        dispatchPowerModeChangedEvent(prevState, newState);

        LOGINFO("keyCode: %d, prevState: %s, newState: %s, reason: %s, errorcode: %u", keyCode, util::str(prevState), util::str(newState), reason.c_str(), errorCode);

        if (PowerState::POWER_STATE_STANDBY_DEEP_SLEEP == newState) {
            LOGINFO("newState is DEEP SLEEP, activating deep sleep mode");
            _powerController.ActivateDeepSleep();
        }

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetPowerState(const int keyCode, const PowerState newState, const string& reason)
    {
        PowerState currState = POWER_STATE_UNKNOWN;
        PowerState prevState = POWER_STATE_UNKNOWN;

        uint32_t errorCode = GetPowerState(currState, prevState);

        if (Core::ERROR_NONE != errorCode) {
            LOGERR("Failed to get current power state, errorCode: %d", errorCode);
            return errorCode;
        }

        if (currState != newState) {

            if (POWER_STATE_STANDBY_DEEP_SLEEP == currState) {
                if (_deepSleepController.IsDeepSleepInProgress()) {
                    LOGINFO("deepsleep in  progress  ignoring %s request", util::str(newState));
                    return Core::ERROR_NONE;
                }
                // deep sleep not in progress, wakeup from deep sleep
                LOGINFO("Device wakeup from DEEP_SLEEP to %s", util::str(newState));
                _deepSleepController.Deactivate();
            }

            _apiLock.Lock();

            if (_modeChangeController) {
                LOGWARN("Power state change is already in progress, cancel old request");
                _modeChangeController.reset();
            }

            _modeChangeController = std::unique_ptr<PreModeChangeController>(new PreModeChangeController());
            const int transactionId = _modeChangeController->TransactionId();

            for (const auto& client : _modeChangeClients) {
                _modeChangeController->AckAwait(client.first);
            }

            _apiLock.Unlock();

            // dispatch pre power mode change notifications
            submitPowerModePreChangeEvent(currState, newState, transactionId);

            // like in `Job` class we avoid impl destruction before handler is invoked
            this->AddRef();

            _modeChangeController->Schedule(POWER_MODE_PRECHANGE_TIMEOUT_SEC * 1000,
                [this, keyCode, currState, newState, reason](bool /*isTimedout*/) mutable {
                    powerModePreChangeCompletionHandler(keyCode, currState, newState, reason);
                });
        } else {
            LOGINFO("Requested power state is same as current power state, no action required");
        }

        LOGINFO("SetPowerState keyCode: %d, currentState: %s, newState: %s, errorCode: %d",
            keyCode, util::str(currState), util::str(newState), Core::ERROR_NONE);

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

        LOGINFO("currentState : %s, newState : %s, transactionId : %d", util::str(currentState), util::str(newState), transactionId);
    }

    Core::hresult PowerManagerImplementation::GetTemperatureThresholds(float& high, float& critical) const
    {
        _apiLock.Lock();

        Core::hresult errorCode = _thermalController.GetTemperatureThresholds(high,critical);

        _apiLock.Unlock();

        LOGINFO("high: %f, critical: %f, errorCode: %u", high, critical, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetTemperatureThresholds(const float high, const float critical)
    {
        _apiLock.Lock();

        Core::hresult errorCode = _thermalController.SetTemperatureThresholds(high,critical);

        _apiLock.Unlock();

        LOGINFO("high: %f, critical: %f, errorCode: %u", high, critical, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetOvertempGraceInterval(int& graceInterval) const
    {
        _apiLock.Lock();

        Core::hresult errorCode = _thermalController.GetOvertempGraceInterval(graceInterval);

        _apiLock.Unlock();

        LOGINFO("graceInterval: %d, errorCode: %u", graceInterval, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetOvertempGraceInterval(const int graceInterval)
    {
        _apiLock.Lock();

        Core::hresult errorCode = _thermalController.SetOvertempGraceInterval(graceInterval);

        _apiLock.Unlock();

        LOGINFO("graceInterval: %d, errorCode: %u", graceInterval, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetThermalState(float& temperature) const
    {
        Core::hresult errorCode = Core::ERROR_GENERAL;
#ifdef ENABLE_THERMAL_PROTECTION
        LOGINFO("Entry");
        _apiLock.Lock();

        ThermalTemperature curLevel = THERMAL_TEMPERATURE_UNKNOWN;
        float curTemperature = 0;

        errorCode = _thermalController.GetThermalState(curLevel,curTemperature);
        temperature = curTemperature;
        _apiLock.Unlock();
        LOGINFO("Current core temperature is : %f, errorCode: %u", temperature, errorCode);
#else
        temperature = -1;
        errorCode = Core::ERROR_GENERAL;
        LOGWARN("Thermal Protection disabled for this platform");
#endif
        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetDeepSleepTimer(const int timeOut)
    {
        _apiLock.Lock();

        uint32_t errorCode = _powerController.SetDeepSleepTimer(timeOut);

        _apiLock.Unlock();

        LOGINFO("timeOut: %d, errorCode: %u", timeOut, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetLastWakeupReason(WakeupReason& wakeupReason) const
    {
        _apiLock.Lock();

        uint32_t errorCode = _deepSleepController.GetLastWakeupReason(wakeupReason);

        _apiLock.Unlock();

        LOGINFO("wakeupReason: %u, errorCode: %u", wakeupReason, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetLastWakeupKeyCode(int& keycode) const
    {
        _apiLock.Lock();

        // TODO: yet to implement
        uint32_t errorCode = _deepSleepController.GetLastWakeupKeyCode(keycode);

        _apiLock.Unlock();

        LOGINFO("Wakeup keycode: %d, errorCode: %u", keycode, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::Reboot(const string& rebootRequestor, const string& rebootReasonCustom, const string& rebootReasonOther)
    {
        const string defaultArg = "Unknown";
        const string requestor = rebootRequestor.empty() ? defaultArg : rebootRequestor;
        const string customReason = rebootReasonCustom.empty() ? defaultArg : rebootReasonCustom;
        const string otherReason = rebootReasonOther.empty() ? defaultArg : rebootReasonOther;

        dispatchRebootBeginEvent(requestor, customReason, otherReason);

        _apiLock.Lock();

        uint32_t errorCode = _powerController.Reboot(rebootRequestor, rebootReasonCustom, rebootReasonOther);

        _apiLock.Unlock();

        LOGINFO("requestor %s, custom reason: %s, other reason: %s, errorcode: %u", requestor.c_str(), customReason.c_str(), otherReason.c_str(), errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetNetworkStandbyMode(const bool standbyMode)
    {
        _apiLock.Lock();

        uint32_t errorCode = _powerController.SetNetworkStandbyMode(standbyMode);

        _apiLock.Unlock();

        LOGINFO("standbyMode : %s, errorcode: %u",
            (standbyMode) ? ("Enabled") : ("Disabled"), errorCode);

        if (Core::ERROR_NONE == errorCode) {
            dispatchNetworkStandbyModeChangedEvent(standbyMode);
        }

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetNetworkStandbyMode(bool& standbyMode)
    {
        _apiLock.Lock();

        uint32_t errorCode = _powerController.GetNetworkStandbyMode(standbyMode);

        _apiLock.Unlock();

        LOGINFO("Current NwStandbyMode is: %s, errorCode: %d",
            (standbyMode ? ("Enabled") : ("Disabled")), errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetWakeupSrcConfig(const int powerMode, const int srcType, int config)
    {
        _apiLock.Lock();

        uint32_t errorCode = _powerController.SetWakeupSrcConfig(powerMode, srcType, config);

        _apiLock.Unlock();

        LOGINFO("Power State stored: %x, srcType: %x,  config: %x, errorCode: %d", powerMode, srcType, config, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetWakeupSrcConfig(int& powerMode, int& srcType, int& config) const
    {
        _apiLock.Lock();

        uint32_t errorCode = _powerController.GetWakeupSrcConfig(powerMode, srcType, config);

        _apiLock.Unlock();

        LOGINFO("Power State stored: %x, srcType: %x,  config: %x, errorCode: %d", powerMode, srcType, config, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::SetSystemMode(const SystemMode currentMode, const SystemMode newMode) const
    {
        _apiLock.Lock();

        // TODO: yet to implement
        uint32_t errorCode = Core::ERROR_GENERAL;

        _apiLock.Unlock();

        LOGINFO("currentMode: %u, newMode: %u, errorCode: %u", currentMode, newMode, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::GetPowerStateBeforeReboot(PowerState& powerStateBeforeReboot)
    {
        _apiLock.Lock();

        uint32_t errorCode = _powerController.GetPowerStateBeforeReboot(powerStateBeforeReboot);

        _apiLock.Unlock();

        LOGWARN("current powerStateBeforeReboot is: %s, errorCode: %d",
            util::str(powerStateBeforeReboot), errorCode);

        return errorCode;
    }

    void PowerManagerImplementation::powerModePreChangeCompletionHandler(const int keyCode, PowerState currentState, PowerState newState, const std::string& reason)
    {
        LOGINFO("PowerModePreChangeCompletionHandler triggered keyCode: %d, powerState: %s", keyCode, util::str(newState));

        setDevicePowerState(keyCode, currentState, newState, reason);

        // release the refCount taken in SetPowerState => _modeChangeController->Schedule
        this->Release();

        // release controller as mode change is complete now
        _modeChangeController.reset();
    }

    Core::hresult PowerManagerImplementation::PowerModePreChangeComplete(const uint32_t clientId, const int transactionId)
    {
        uint32_t errorCode = Core::ERROR_INVALID_PARAMETER;

        _apiLock.Lock();

        if (_modeChangeController) {
            errorCode = _modeChangeController->Ack(clientId, transactionId);
        }

        _apiLock.Unlock();

        LOGINFO("clientId: %u, transactionId: %d, errorcode: %u", clientId, transactionId, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::DelayPowerModeChangeBy(const uint32_t clientId, const int transactionId, const int delayPeriod)
    {
        uint32_t errorCode = Core::ERROR_INVALID_PARAMETER;

        _apiLock.Lock();

        if (_modeChangeController) {
            errorCode = _modeChangeController->Reschedule(clientId, transactionId, delayPeriod * 1000);
        }

        _apiLock.Unlock();

        LOGINFO("DelayPowerModeChangeBy clientId: %u, transactionId: %d, delayPeriod: %d, errorcode: %u", clientId, transactionId, delayPeriod, errorCode);

        return errorCode;
    }

    Core::hresult PowerManagerImplementation::AddPowerModePreChangeClient(const string& clientName, uint32_t& clientId)
    {
        if (clientName.empty()) {
            LOGERR("AddPowerModePreChangeClient called with empty clientName");
            return Core::ERROR_INVALID_PARAMETER;
        }

        _apiLock.Lock();

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

        _apiLock.Unlock();

        LOGINFO("client: %s, clientId: %u", clientName.c_str(), clientId);

        for (auto& clients : _modeChangeClients) {
            LOGINFO("Registered client: %s, clientId: %u", clients.second.c_str(), clients.first);
        }

        return Core::ERROR_NONE;
    }

    Core::hresult PowerManagerImplementation::RemovePowerModePreChangeClient(const uint32_t clientId)
    {
        uint32_t errorCode = Core::ERROR_INVALID_PARAMETER;
        std::string clientName;

        _apiLock.Lock();

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

        _apiLock.Unlock();

        LOGINFO("client: %s, clientId: %u, errorcode: %u", clientName.c_str(), clientId, errorCode);

        return errorCode;
    }

    void PowerManagerImplementation::onDeepSleepTimerWakeup(const int wakeupTimeout)
    {
        LOGINFO("DeepSleep timedout: %d", wakeupTimeout);
        dispatchDeepSleepTimeoutEvent(wakeupTimeout);

#if !defined(_DISABLE_SCHD_REBOOT_AT_DEEPSLEEP)
        LOGINFO("Reboot the box due to Deep Sleep Timer Expiry : %d", wakeupTimeout);
        _deepSleepController.MaintenanceReboot();
#else
        /*Scheduled maintanace reboot is disabled. Instead state will change to LIGHT_SLEEP*/
        LOGINFO("Set Device to light sleep on Deep Sleep timer expiry");
        SetPowerState(0, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP, "DeepSleep timedout");
#endif // _DISABLE_SCHD_REBOOT_AT_DEEPSLEEP
    }

    void PowerManagerImplementation::onDeepSleepUserWakeup(const bool userWakeup)
    {
        PowerState newState = PowerState::POWER_STATE_ON;

#ifdef PLATCO_BOOTTO_STANDBY
        newState = PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP;
#endif
        LOGINFO("User triggered wakeup from DEEP_SLEEP, moving to powerState: %s", util::str(newState));
        SetPowerState(0, newState, "DeepSleep userwakeup");
    }

    void PowerManagerImplementation::onDeepSleepFailed()
    {
        PowerState newState = PowerState::POWER_STATE_ON;

#ifdef PLATCO_BOOTTO_STANDBY
        newState = PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP;
#endif
        LOGINFO("Failed to enter DeepSleep, moving to powerState: %s", util::str(newState));
        SetPowerState(0, newState, "DeepSleep failed");
    }

    void PowerManagerImplementation::onThermalTemperatureChanged(const ThermalTemperature cur_Thermal_Level,
                                                                const ThermalTemperature new_Thermal_Level, const float current_Temp)
    {
        LOGINFO("THERMAL_MODECHANGED event received, curLevel: %u, newLevel: %u, curTemperature: %f",
                cur_Thermal_Level, new_Thermal_Level, current_Temp);

        dispatchThermalModeChangedEvent(cur_Thermal_Level, new_Thermal_Level, current_Temp);
    }

    void PowerManagerImplementation::onDeepSlepForThermalChange()
    {
        LOGINFO("Entry");

        /*Scheduled maintanace reboot is disabled. Instead state will change to LIGHT_SLEEP*/
        LOGINFO("Set Device to deep sleep on Thermal change");
        SetPowerState(0, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "DeepSleep on Thermal change");
    }

}
}
