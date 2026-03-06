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

#include <functional>
#include <unistd.h>

#include <core/IAction.h>
#include <core/Time.h>
#include <core/WorkerPool.h>

#include "UtilsLogging.h"
#include "secure_wrapper.h"
#include "fpd.h"

FPD::FPD(INotification& parent, std::shared_ptr<IPlatform> platform)
    : _platform(std::move(platform))
    , _parent(parent)
{
    ENTRY_LOG;
    LOGINFO("FPD Constructor");
    Platform_init();
    EXIT_LOG;
}

void FPD::Platform_init()
{
    ENTRY_LOG;
    // Initialize FPD platform
    LOGINFO("FPD Init");
    EXIT_LOG;
}

//Depricated
uint32_t FPD::SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) {
    ENTRY_LOG;
    LOGINFO("SetFPDTime: timeFormat=%d, minutes=%u, seconds=%u", timeFormat, minutes, seconds);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetFPDTime(timeFormat, minutes, seconds);
    }
    EXIT_LOG;
    return result;
}

uint32_t FPD::SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) {
    ENTRY_LOG;
    LOGINFO("SetFPDScroll: scrollHoldDuration=%u, horizontal=%u, vertical=%u", scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetFPDScroll(scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations);
    }
    EXIT_LOG;
    return result;
}

uint32_t FPD::SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) {
    ENTRY_LOG;
    LOGINFO("SetFPDTextBrightness: textDisplay=%d, brightNess=%u", textDisplay, brightNess);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetFPDTextBrightness(textDisplay, brightNess);
    }
    EXIT_LOG;
    return result;
}

uint32_t FPD::GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) {
    ENTRY_LOG;
    LOGINFO("GetFPDTextBrightness: textDisplay=%d", textDisplay);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetFPDTextBrightness(textDisplay, brightNess);
    }
    EXIT_LOG;
    return result;
}

uint32_t FPD::EnableFPDClockDisplay(const bool enable) {
    ENTRY_LOG;
    LOGINFO("EnableFPDClockDisplay: enable=%s", enable ? "true" : "false");
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().EnableFPDClockDisplay(enable);
    }
    EXIT_LOG;
    return result;
}

uint32_t FPD::GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) {
    ENTRY_LOG;
    LOGINFO("GetFPDTimeFormat");
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetFPDTimeFormat(fpdTimeFormat);
    }
    EXIT_LOG;
    return result;
}

uint32_t FPD::SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) {
    ENTRY_LOG;
    LOGINFO("SetFPDTimeFormat: fpdTimeFormat=%d", fpdTimeFormat);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetFPDTimeFormat(fpdTimeFormat);
    }
    EXIT_LOG;
    return result;
}
//Depricated

uint32_t FPD::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
    ENTRY_LOG;

    LOGINFO("SetFPDBlink: indicator=%d, blinkDuration=%u, blinkIterations:%u", indicator, blinkDuration, blinkIterations);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetFPDBlink(indicator, blinkDuration, blinkIterations);
    }

    EXIT_LOG;
    return result;
}

uint32_t FPD::GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) {
    ENTRY_LOG;

    LOGINFO("GetFPDBrightness: indicator=%d", indicator);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetFPDBrightness(indicator, brightNess);
        LOGINFO("GetFPDBrightness: indicator=%d, brightNess=%d", indicator, brightNess);
    }

    EXIT_LOG;
    return result;
}

uint32_t FPD::SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) {
    ENTRY_LOG;

    LOGINFO("SetFPDBrightness: indicator=%d, brightNess=%u, persist=%s", indicator, brightNess, persist ? "true" : "false");
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetFPDBrightness(indicator, brightNess, persist);
    }

    EXIT_LOG;
    return result;
}

uint32_t FPD::GetFPDState(const FPDIndicator indicator, FPDState &state) {
    ENTRY_LOG;

    LOGINFO("GetFPDState: indicator=%d", indicator);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetFPDState(indicator, state);
        LOGINFO("GetFPDState: indicator=%d, state=%d", indicator, state);
    }

    EXIT_LOG;
    return result;
}

uint32_t FPD::SetFPDState(const FPDIndicator indicator, const FPDState state) {
    ENTRY_LOG;

    LOGINFO("SetFPDState: indicator=%d, state=%d", indicator, state);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetFPDState(indicator, state);
    }

    EXIT_LOG;
    return result;
}

uint32_t FPD::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
    ENTRY_LOG;

    LOGINFO("GetFPDColor: indicator=%d", indicator);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetFPDColor(indicator, color);
        LOGINFO("GetFPDColor: indicator=%d, colour=%d", indicator, color);
    }
    EXIT_LOG;
    return result;
}

uint32_t FPD::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
    ENTRY_LOG;

    LOGINFO("SetFPDColor: indicator=%d, colour=%d", indicator, color);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetFPDColor(indicator, color);
        LOGINFO("SetFPDColor: indicator=%d, colour=%d - result=%d", indicator, color, result);
    }

    EXIT_LOG;
    return result;
}

uint32_t FPD::SetFPDMode(const FPDMode fpdMode) {
    ENTRY_LOG;
    LOGINFO("SetFPDMode: fpdMode=%d", fpdMode);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetFPDMode(fpdMode);
    }
    EXIT_LOG;
    return result;
}
