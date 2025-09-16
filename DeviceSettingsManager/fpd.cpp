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

#include <functional> // for function
#include <unistd.h>   // for access, F_OK

#include <core/IAction.h>    // for IDispatch
#include <core/Time.h>       // for Time
#include <core/WorkerPool.h> // for IWorkerPool, WorkerPool

#include "UtilsLogging.h"   // for LOGINFO, LOGERR
#include "secure_wrapper.h" // for v_secure_system
#include "fpd.h"
//#include "dsfpd.h"

/*static  dsFPDBrightness_t _dsPowerBrightness = 100;
static  dsFPDBrightness_t _dsTextBrightness  = 100;
static  dsFPDColor_t      _dsPowerLedColor   = dsFPD_COLOR_BLUE;
static  dsFPDTimeFormat_t _dsTextTimeFormat	 = dsFPD_TIME_12_HOUR;
static  dsFPDMode_t       _dsFPDMode         = dsFPD_MODE_ANY;*/

FPD::FPD() : _workerPool(WPEFramework::Core::WorkerPool::Instance())
{
    ENTRY_LOG;
    LOGINFO("FPD Constructor");
    init();
    EXIT_LOG;
}

void FPD::init()
{
    ENTRY_LOG;
    LOGINFO("FPD Init");
    EXIT_LOG;
}

//Depricated
uint32_t FPD::SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) {
    ENTRY_LOG;
    LOGINFO("SetFPDTime: timeFormat=%d, minutes=%u, seconds=%u", timeFormat, minutes, seconds);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) {
    ENTRY_LOG;
    LOGINFO("SetFPDScroll: scrollHoldDuration=%u, horizontal=%u, vertical=%u", scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) {
    ENTRY_LOG;
    LOGINFO("SetFPDTextBrightness: textDisplay=%d, brightNess=%u", textDisplay, brightNess);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) {
    ENTRY_LOG;
    LOGINFO("GetFPDTextBrightness: textDisplay=%d", textDisplay);
    brightNess = 50; // Example value
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::EnableFPDClockDisplay(const bool enable) {
    ENTRY_LOG;
    LOGINFO("EnableFPDClockDisplay: enable=%s", enable ? "true" : "false");
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) {
    ENTRY_LOG;
    LOGINFO("GetFPDTimeFormat");
    fpdTimeFormat = FPDTimeFormat::DS_FPD_TIMEFORMAT_24_HOUR; // Example value
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) {
    ENTRY_LOG;
    LOGINFO("SetFPDTimeFormat: fpdTimeFormat=%d", fpdTimeFormat);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}
//Depricated

uint32_t FPD::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
    ENTRY_LOG;
    LOGINFO("SetFPDBlink: indicator=%d, blinkDuration=%u, blinkIterations=%u", indicator, blinkDuration, blinkIterations);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) {
    ENTRY_LOG;

    dsFPDBrightParam_t param;
    param.eIndicator = static_cast<dsFPDIndicator_t>(indicator);
    param.eBrightness = static_cast<dsFPDBrightness_t>(brightNess);
    param.toPersist = static_cast<bool>(persist);
    LOGINFO("SetFPDBrightness: indicator=%d, brightNess=%u, persist=%s", indicator, brightNess, persist ? "true" : "false");
    _dsSetFPBrightness(static_cast<void*>(&param));

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) {
    ENTRY_LOG;

    dsFPDBrightParam_t param;
    param.eIndicator = static_cast<dsFPDIndicator_t>(indicator);
    param.eBrightness = brightNess;
    LOGINFO("GetFPDBrightness: indicator=%d, brightNess=%d", indicator, brightNess);
    _dsGetFPBrightness(static_cast<void*>(&param));
    brightNess = param.eBrightness;
    LOGINFO("GetFPDBrightness: indicator=%d, brightNess=%d", indicator, brightNess);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDState(const FPDIndicator indicator, const FPDState state) {
    ENTRY_LOG;

    dsFPDStateParam_t param;
    LOGINFO("SetFPDState: indicator=%d, state=%d", indicator, state);
    param.state = static_cast<dsFPDState_t>(state);
    param.eIndicator = static_cast<dsFPDIndicator_t>(indicator);
    _dsSetFPState(static_cast<void*>(&param));

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDState(const FPDIndicator indicator, FPDState &state) {
    ENTRY_LOG;

    LOGINFO("GetFPDState: indicator=%d", indicator);
    dsFPDStateParam_t param;
    LOGINFO("GetFPDState: indicator=%d, state=%d", indicator, state);
    param.state = static_cast<dsFPDState_t>(state);
    param.eIndicator = static_cast<dsFPDIndicator_t>(indicator);
    _dsGetFPState(static_cast<void*>(&param));
    //state = FPDState::DS_FPD_STATE_ON;
    state = static_cast<FPDState>(param.state);
    LOGINFO("GetFPDState: indicator=%d, state=%d", indicator, state);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
    ENTRY_LOG;
    LOGINFO("GetFPDColor: indicator=%d", indicator);
    color = 0xFFFFFF; // Example value
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
    ENTRY_LOG;
    LOGINFO("SetFPDColor: indicator=%d, color=0x%X", indicator, color);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDMode(const FPDMode fpdMode) {
    ENTRY_LOG;
    LOGINFO("SetFPDMode: fpdMode=%d", fpdMode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}
