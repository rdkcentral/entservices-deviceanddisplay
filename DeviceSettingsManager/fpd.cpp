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

extern IARM_Result_t _dsGetFPBrightness(void *arg);
extern IARM_Result_t _dsSetFPBrightness(void *arg);
extern IARM_Result_t _dsSetFPState(void *arg);
extern IARM_Result_t _dsGetFPState(void *arg);
extern IARM_Result_t _dsSetFPColor(void *arg);
extern IARM_Result_t _dsGetFPColor(void *arg);
extern IARM_Result_t _dsSetFPBlink(void *arg);

FPD::FPD(INotification& parent, std::shared_ptr<IPlatform> platform)
    : _platform(std::move(platform))
    , _parent(parent)
{
    ENTRY_LOG;
    LOGINFO("FPD Constructor");
    LOGINFO("HDMI version: %s\n", HdmiConnectionToStrMapping[0].name);
    LOGINFO("HDMI version: %s\n", HdmiStatusToStrMapping[0].name);
    LOGINFO("HDMI version: %s\n", HdmiVerToStrMapping[0].name);
    Platform_init();
    EXIT_LOG;
}

void FPD::Platform_init()
{
    ENTRY_LOG;

    if (_platform) {
    //this->platform().setAllCallbacks(bundle);
    //this->platform().getPersistenceValue();
    }

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

    dsFPDBlinkParam_t param;
    param.eIndicator = static_cast<dsFPDIndicator_t>(indicator);
    param.nBlinkDuration = blinkDuration;
    param.nBlinkIterations = blinkIterations;
    LOGINFO("SetFPDBlink: indicator=%d, blinkDuration=%u, blinkIterations:%u", indicator, blinkDuration, blinkIterations);
    _dsSetFPBlink(static_cast<void*>(&param));

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

uint32_t FPD::GetFPDState(const FPDIndicator indicator, FPDState &state) {
    ENTRY_LOG;

    LOGINFO("GetFPDState: indicator=%d", indicator);
    dsFPDStateParam_t param;
    param.state = static_cast<dsFPDState_t>(indicator);
    param.eIndicator = static_cast<dsFPDIndicator_t>(indicator);
    LOGINFO("GetFPDState: indicator=%d, state=%d", param.eIndicator, param.state);
    _dsGetFPState(static_cast<void*>(&param));
    //state = FPDState::DS_FPD_STATE_ON;
    state = static_cast<FPDState>(param.state);
    LOGINFO("GetFPDState: indicator=%d, state=%d", indicator, state);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDState(const FPDIndicator indicator, const FPDState state) {
    ENTRY_LOG;

    dsFPDStateParam_t param;
    LOGINFO("SetFPDState: indicator=%d, state=%d", indicator, state);
    param.state = static_cast<dsFPDState_t>(indicator);
    param.eIndicator = static_cast<dsFPDIndicator_t>(indicator);
    _dsSetFPState(static_cast<void*>(&param));

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
    ENTRY_LOG;

    dsFPDColorParam_t param;
    param.eIndicator = static_cast<dsFPDIndicator_t>(indicator);
    param.eColor = color;
    LOGINFO("GetFPDState: indicator=%d, colour=%d", param.eIndicator, param.eColor);
    _dsGetFPColor(static_cast<void*>(&param));
    LOGINFO("GetFPDState: indicator=%d, colour=%d", param.eIndicator, param.eColor);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
    ENTRY_LOG;

    dsFPDColorParam_t param;
    param.eIndicator = static_cast<dsFPDIndicator_t>(indicator);
    param.eColor = color;
    LOGINFO("GetFPDState: indicator=%d, colour=%d", param.eIndicator, param.eColor);
    _dsSetFPColor(static_cast<void*>(&param));
    LOGINFO("GetFPDState: indicator=%d, colour=%d", param.eIndicator, param.eColor);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FPD::SetFPDMode(const FPDMode fpdMode) {
    ENTRY_LOG;
    LOGINFO("SetFPDMode: fpdMode=%d", fpdMode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}
