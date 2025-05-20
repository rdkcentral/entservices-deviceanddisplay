/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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

#include <chrono>
#include <errno.h>    // for errno
#include <functional> // for function
#include <stdio.h>    // for fclose, fopen, fscanf, FILE, ferror
#include <stdlib.h>   // for system, rand, srand
#include <string.h>   // for strerror, strlen
#include <sys/stat.h> // for stat
#include <time.h>     // for tm, time, difftime, mktime, NULL, loca...
#include <unistd.h>   // for sleep

#include <core/IAction.h>     // for IDispatch
#include <core/Portability.h> // for ErrorCodes
#include <core/Time.h>        // for Time
#include <core/WorkerPool.h>  // for IWorkerPool, WorkerPool

#include "DeepSleepController.h"
#include "LambdaJob.h"    // for LambdaJob
#include "UtilsLogging.h" // for LOGINFO, LOGERR
#include "libIARM.h"      // for _IARM_Result_t, IARM_Result_t
#include "libIBus.h"      // for IARM_Bus_Call
#include "sysMgr.h"       // for IARM_BUS_SYSMGR_API_GetSystemStates

using WakeupReason = WPEFramework::Exchange::IPowerManager::WakeupReason;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
using IPlatform = hal::deepsleep::IPlatform;

std::map<std::string, DeepSleepWakeupSettings::tzValue> DeepSleepWakeupSettings::_maptzValues;

uint32_t DeepSleepWakeupSettings::getTZDiffInSec() const
{
    uint32_t _TZDiffTime = 6 * 3600;
    IARM_Result_t iResult = IARM_RESULT_SUCCESS;
    tzValue value = tzCST06;

    /* Get the Time Zone Pay Load from SysMgr */
    IARM_Bus_SYSMgr_GetSystemStates_Param_t param;
    iResult = IARM_Bus_Call(IARM_BUS_SYSMGR_NAME, IARM_BUS_SYSMGR_API_GetSystemStates, (void*)&param, sizeof(param));
    if (iResult == IARM_RESULT_SUCCESS) {
        if (param.time_zone_available.error) {
            LOGINFO("Failed to get the Time Zone Information from SysMgr");
        } else if (param.time_zone_available.state == 2) {
            if (strlen(param.time_zone_available.payload) > 1) {
                LOGINFO("TZ Payload - %s", param.time_zone_available.payload);
                value = _maptzValues[param.time_zone_available.payload];
                _TZDiffTime = value * 3600;

                LOGINFO("TZ value = %d", value);
                LOGINFO("Time Zone in Sec = %d", _TZDiffTime);
            }
        }
    }
    return _TZDiffTime;
}

/*  Get TZ diff
    Have Record of All US TZ as of now.
*/
void DeepSleepWakeupSettings::initializeTimeZone()
{
    _maptzValues["HST11"] = tzHST11;
    _maptzValues["HST11HDT,M3.2.0,M11.1.0"] = tzHST11HDT;
    _maptzValues["AKST"] = tzAKST;
    _maptzValues["AKST09AKDT,M3.2.0,M11.1.0"] = tzAKST09AKDT;
    _maptzValues["PST08"] = tzPST08;
    _maptzValues["PST08PDT,M3.2.0,M11.1.0"] = tzPST08PDT;
    _maptzValues["MST07"] = tzMST07;
    _maptzValues["MST07MDT,M3.2.0,M11.1.0"] = tzMST07MDT;
    _maptzValues["CST06"] = tzCST06;
    _maptzValues["CST06CDT,M3.2.0,M11.1.0"] = tzCST06CDT;
    _maptzValues["EST05"] = tzEST05;
    _maptzValues["EST05EDT,M3.2.0,M11.1.0"] = tzEST05EDT;
}

/*  Get Wakeup timeout.
    Wakeup the box to do Maintenance related activities.
*/
uint32_t DeepSleepWakeupSettings::getWakeupTime() const
{
    time_t now, wakeup;
    struct tm wakeupTime;
    uint32_t wakeupTimeInSec = 0, getTZDiffTime = 0;
    uint32_t wakeupTimeInMin = 5;

    /* Read the wakeup Time in Seconds from /tmp override
       else calculate the Wakeup time til 2AM */
    FILE* fp = fopen("/tmp/deepSleepWakeupTimer", "r");
    if (NULL != fp) {
        if (0 > fscanf(fp, "%d", &wakeupTimeInMin)) {
            LOGINFO("Error: fscanf on wakeupTimeInSec failed");
        } else {
            wakeupTimeInSec = wakeupTimeInMin * 60;
            fclose(fp);
            LOGINFO("/tmp/ override Deep Sleep Wakeup Time is %d", wakeupTimeInSec);
            return wakeupTimeInSec;
        }
        fclose(fp);
    }

    /* curr time */
    time(&now);

    /* wakeup time */
    time(&wakeup);
    wakeupTime = *localtime(&wakeup);

    if (wakeupTime.tm_hour >= 0 && wakeupTime.tm_hour < 2) {
        /*Calculate the wakeup time till 2 AM..*/
        wakeupTime.tm_hour = 2;
        wakeupTime.tm_min = 0;
        wakeupTime.tm_sec = 0;
        wakeupTimeInSec = difftime(mktime(&wakeupTime), now);

    } else {
        /*Calculate the wakeup time till midnight + 2 hours for 2 AM..*/
        wakeupTime.tm_hour = 23;
        wakeupTime.tm_min = 59;
        wakeupTime.tm_sec = 60;
        wakeupTimeInSec = difftime(mktime(&wakeupTime), now);
        wakeupTimeInSec = wakeupTimeInSec + 7200; // 7200sec for 2 hours
    }

    /* Add randomness to calculated value i.e between 2AM - 3AM
        for 1 hour window
    */
    srand(time(NULL));
    uint32_t randTimeInSec = (uint32_t)rand() % (3600) + 0; // for 1 hour window
    wakeupTimeInSec = wakeupTimeInSec + randTimeInSec;
    LOGINFO("Calculated Deep Sleep Wakeup Time Before TZ setting is %d Sec", wakeupTimeInSec);

    getTZDiffTime = getTZDiffInSec();
    wakeupTimeInSec = wakeupTimeInSec + getTZDiffTime;

    LOGINFO("Calculated Deep Sleep Wakeup Time After TZ setting is %d Sec", wakeupTimeInSec);

    return wakeupTimeInSec;
}

DeepSleepController::DeepSleepController(INotification& parent, std::unique_ptr<IPlatform> platform)
    : _parent(parent)
    , _workerPool(WPEFramework::Core::WorkerPool::Instance())
    , _platform(std::move(platform))
    , _deepSleepState(DeepSleepState::NotStarted)
    , _deepSleepDelaySec(0)
    , _deepSleepWakeupTimeoutSec(0)
    , _nwStandbyMode(false)
{
}

uint32_t DeepSleepController::GetLastWakeupReason(WakeupReason& wakeupReason) const
{
    return platform().GetLastWakeupReason(wakeupReason);
}

uint32_t DeepSleepController::GetLastWakeupKeyCode(int& keyCode) const
{
    return platform().GetLastWakeupKeyCode(keyCode);
}

// activate deep sleep mode
uint32_t DeepSleepController::Activate(uint32_t timeOut, bool nwStandbyMode)
{
    LOGINFO("timeOut: %u, nwStandbyMode: %s", timeOut, (nwStandbyMode ? "Enabled" : "Disabled"));
    _workerPool.Submit(LambdaJob::Create([this, timeOut, nwStandbyMode]() {
        LOGINFO("timeOut: %u, nwStandbyMode: %s", timeOut, (nwStandbyMode ? "Enabled" : "Disabled"));
        performActivate(timeOut, nwStandbyMode);
    }));

    return WPEFramework::Core::ERROR_NONE;
}

// deactivate deep sleep mode
uint32_t DeepSleepController::Deactivate()
{
    if (_deepSleepDelayJob.IsValid()) {
        // Cancel the delay timer if it is still active
        _workerPool.Revoke(_deepSleepDelayJob);
        _deepSleepDelayJob.Release();
        LOGINFO("Deepsleep delayed job cancelled");
    }

    if (_deepSleepWakeupJob.IsValid()) {
        // Cancel the wakeup timer if it is still active
        _workerPool.Revoke(_deepSleepWakeupJob);
        _deepSleepWakeupJob.Release();
        LOGINFO("Deepsleep wakeup job cancelled");
    }

    uint32_t errorCode = platform().DeepSleepWakeup();

    _deepSleepState = DeepSleepState::NotStarted;

    LOGINFO("Deepsleep wakeup completed, errorCode: %u", errorCode);

    return errorCode;
}

bool DeepSleepController::read_integer_conf(const char* file_name, uint32_t& val)
{
    bool ok = false;
    FILE* file = fopen(file_name, "r");
    const char* err = nullptr;

    if (file != nullptr) {
        int res = fscanf(file, "%u", &val);
        if (1 == res) {
            ok = true;
        } else {
            err = ferror(file) ? strerror(errno) : "fscanf EOF";
        }
        fclose(file);
    } else {
        err = strerror(errno);
    }

    if (!ok) {
        LOGERR("file %s, error: %s", file_name, err);
    }

    return ok;
}

void DeepSleepController::enterDeepSleepDelayed()
{
    _deepSleepDelayJob.Release();

    struct stat buf;

    LOGINFO("Deep Sleep Timer Expires :Enter to Deep sleep Mode..stop Receiver with sleep 10 before DS");
    // To-do:
    system("sleep 10");

    if ((stat("/lib/systemd/system/lxc.service", &buf) == 0) && (stat("/opt/lxc_service_disabled", &buf) != 0)) {
        system("systemctl stop lxc.service");
    }

    bool userWakeup = 0;

    auto startTime = std::chrono::steady_clock::now();
    auto status = platform().SetDeepSleep(_deepSleepWakeupTimeoutSec, userWakeup, false);

    if (WPEFramework::Core::ERROR_NONE != status) {
        LOGINFO("Failed to enter deepsleep status %u", status);
        _deepSleepState = DeepSleepState::Failed;
        _parent.onDeepSleepFailed();
        return;
    }

    _deepSleepState = DeepSleepState::Completed;

    if (userWakeup) {
        LOGINFO("DeeSleep wakeupReason: user action");
        _parent.onDeepSleepUserWakeup(userWakeup);
    } else {
        deepSleepTimerWakeup(startTime);
    }
}

void DeepSleepController::enterDeepSleepNow()
{
    LOGINFO("Enter to Deep sleep Mode..stop Receiver with sleep 2 before DS");
    system("sleep 2");
    bool failed = true;
    int retryCount = 5;
    bool userWakeup = 0;
    std::chrono::steady_clock::time_point startTime;

    while (retryCount && failed) {
        LOGINFO("Device entering Deep sleep with nwStandbyMode: %s",
            (_nwStandbyMode ? "Enabled" : "Disabled"));

        startTime = std::chrono::steady_clock::now();
        uint32_t errorCode = platform().SetDeepSleep(_deepSleepWakeupTimeoutSec, userWakeup, _nwStandbyMode);

        failed = WPEFramework::Core::ERROR_NONE != errorCode;

        if (failed) {
            _deepSleepState = DeepSleepState::Failed;
            retryCount--;

            if (retryCount) {
                LOGINFO("Failed to enter deep sleep mode: %u, retry after 5s", errorCode);
                sleep(5);
            }
        } else {
            _deepSleepState = DeepSleepState::Completed;
            LOGINFO("Device entered to Deep sleep Mode..");
        }
    }

    if (failed) {
        LOGERR("Failed to enter deep sleep mode after 5 attempts");
        _parent.onDeepSleepFailed();
        return;
    }

    if (userWakeup) {
        LOGINFO("DeeSleep wakeupReason: user action");
        _parent.onDeepSleepUserWakeup(userWakeup);
    } else {
        deepSleepTimerWakeup(startTime);
    }
}

void DeepSleepController::deepSleepTimerWakeup(const std::chrono::steady_clock::time_point& startTime)
{
#ifdef USE_WAKEUP_TIMER_EVT
    WakeupReason wakeupReason = WakeupReason::WAKEUP_REASON_UNKNOWN;
    uint32_t errorCode = platform().GetLastWakeupReason(wakeupReason);
    if (WPEFramework::Core::ERROR_NONE == errorCode && WakeupReason::WAKEUP_REASON_TIMER == wakeupReason) {
        LOGINFO("DeepSleep wakeupReason: %d, timeout: %d", wakeupReason, _deepSleepWakeupTimeoutSec);
        _parent.onDeepSleepTimerWakeup(_deepSleepWakeupTimeoutSec);
        return;
    }
#endif
    auto elapsed = std::chrono::steady_clock::now() - startTime;

    if (elapsed >= std::chrono::seconds(_deepSleepWakeupTimeoutSec)) {
        LOGINFO("DeepSleep wakeupReason: TIMER, timeout: %d", _deepSleepWakeupTimeoutSec);
        _parent.onDeepSleepTimerWakeup(_deepSleepWakeupTimeoutSec);
    } else {
        auto pending = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(_deepSleepWakeupTimeoutSec) - elapsed).count();

        LOGERR("DeepSleep wakeupReason: UNKNOWN, timeout: %ds, elapsed: %llds, pending: %lldms",
            _deepSleepWakeupTimeoutSec, std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), pending);

        auto pendingTime = WPEFramework::Core::Time::Now().Add(pending);

        _deepSleepWakeupJob = LambdaJob::Create([this]() {
            LOGINFO("DeepSleep wakeupReason: TIMER (delayed), timeout: %d", _deepSleepWakeupTimeoutSec);
            _parent.onDeepSleepTimerWakeup(_deepSleepWakeupTimeoutSec);
        });

        _workerPool.Schedule(pendingTime, _deepSleepWakeupJob);
    }
}

void DeepSleepController::MaintenanceReboot()
{
#if !defined(_DISABLE_SCHD_REBOOT_AT_DEEPSLEEP)
    LOGINFO("Scheduled maintanace reboot is enabled");
    system("echo 0 > /opt/.rebootFlag");
    system("echo `/bin/timestamp` ------------- Reboot timer expired while in Deep Sleep --------------- >> /opt/logs/receiver.log");
    system("sleep 5; /rebootNow.sh -s DeepSleepMgr -o 'Rebooting the box due to reboot timer expired while in Deep Sleep...'");
#endif // !_DISABLE_SCHD_REBOOT_AT_DEEPSLEEP
}

void DeepSleepController::performActivate(uint32_t timeOut, bool nwStandbyMode)
{
    LOGINFO("timeOut: %u, nwStandbyMode: %s", timeOut, (nwStandbyMode ? "Enabled" : "Disabled"));
    if (!IsDeepSleepInProgress()) {

        // latch
        _deepSleepState = DeepSleepState::InProgress;

        // Perform the deep sleep operation
        _deepSleepWakeupTimeoutSec = timeOut;

        uint32_t delayTimeOut = 0;
        if (read_integer_conf("/tmp/deepSleepTimer", delayTimeOut) && delayTimeOut) {
            _deepSleepDelaySec = delayTimeOut;
            LOGINFO("/tmp/deepSleepTimer override Deep Sleep timeOut value: %u", delayTimeOut);
        }

        uint32_t wakeupTimer = 0;
        if (read_integer_conf("/tmp/deepSleepTimerVal", wakeupTimer) && wakeupTimer) {
            _deepSleepWakeupTimeoutSec = wakeupTimer;
            LOGINFO("/tmp/deepSleepTimerVal override Deep Sleep wakeup timer value: %u", wakeupTimer);
        }

        if (_deepSleepDelaySec) {
            _deepSleepDelayJob = LambdaJob::Create([this]() {
                enterDeepSleepDelayed();
            });

            WPEFramework::Core::WorkerPool::Instance().Schedule(
                WPEFramework::Core::Time(
                    WPEFramework::Core::Time::Now().Add(_deepSleepDelaySec * 1000)),
                _deepSleepDelayJob);
        } else {
            enterDeepSleepNow();
        }
    } else {
        LOGERR("Deep sleep operation is already in progress");
    }
}
