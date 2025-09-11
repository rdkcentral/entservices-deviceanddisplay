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
    LOGINFO("FPD Constructor");
    init();
}

void FPD::init()
{
    LOGINFO("FPD Init");
}

//Depricated
uint32_t FPD::SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) {
    LOGINFO("SetFPDTime: timeFormat=%d, minutes=%u, seconds=%u", timeFormat, minutes, seconds);
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) {
    LOGINFO("SetFPDScroll: scrollHoldDuration=%u, horizontal=%u, vertical=%u", scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations);
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) {
    LOGINFO("SetFPDTextBrightness: textDisplay=%d, brightNess=%u", textDisplay, brightNess);
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) {
    LOGINFO("GetFPDTextBrightness: textDisplay=%d", textDisplay);
    brightNess = 50; // Example value
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::EnableFPDClockDisplay(const bool enable) {
    LOGINFO("EnableFPDClockDisplay: enable=%s", enable ? "true" : "false");
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) {
    LOGINFO("GetFPDTimeFormat");
    fpdTimeFormat = FPDTimeFormat::DS_FPD_TIMEFORMAT_24_HOUR; // Example value
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) {
    LOGINFO("SetFPDTimeFormat: fpdTimeFormat=%d", fpdTimeFormat);
    return WPEFramework::Core::ERROR_NONE;
}
//Depricated

uint32_t FPD::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
    LOGINFO("SetFPDBlink: indicator=%d, blinkDuration=%u, blinkIterations=%u", indicator, blinkDuration, blinkIterations);
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) {
    LOGINFO("SetFPDBrightness: indicator=%d, brightNess=%u, persist=%s", indicator, brightNess, persist ? "true" : "false");
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) {
    dsFPDBrightParam_t *param = nullptr;
    param->eIndicator = static_cast<dsFPDIndicator_t>(indicator);
    param->eBrightness = brightNess;
    LOGINFO("GetFPDBrightness: indicator=%d", indicator);
    _dsGetFPBrightness(static_cast<void*>(param));
    //dsGetFPBrightness(static_cast<dsFPDIndicator_t>(indicator), &brightNess);
    //dsGetFPBrightness(indicator, &brightNess);
    LOGINFO("GetFPDBrightness: indicator=%d, brightNess=%d", indicator, brightNess);
    //brightNess = 50; // Example value
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDState(const FPDIndicator indicator, const FPDState state) {
    LOGINFO("SetFPDState: indicator=%d, state=%d", indicator, state);
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDState(const FPDIndicator indicator, FPDState &state) {
    LOGINFO("GetFPDState: indicator=%d", indicator);
    state = FPDState::DS_FPD_STATE_ON; // Example value
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
    LOGINFO("GetFPDColor: indicator=%d", indicator);
    color = 0xFFFFFF; // Example value
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
    LOGINFO("SetFPDColor: indicator=%d, color=0x%X", indicator, color);
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDMode(const FPDMode fpdMode) {
    LOGINFO("SetFPDMode: fpdMode=%d", fpdMode);
    return WPEFramework::Core::ERROR_NONE;
}

