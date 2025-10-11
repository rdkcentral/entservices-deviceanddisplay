// Out-of-line virtual destructor definition for RTTI/typeinfo
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
#pragma once

#include <cstdint>
#include <cstdio>
#include "dFPD.h"
#include "deviceUtils.h"
#include "dsHdmiIn.h"
#include "dsError.h"
#include "dsHdmiInTypes.h"
#include "dsUtl.h"
#include "dsTypes.h"
#include "dsInternal.h"
#include "dsRpc.h"
#include "dsFPD.h"
#include "dsFPDTypes.h"
#include "UtilsLogging.h"

#include <WPEFramework/interfaces/IDeviceSettingsManager.h>

using FPDTimeFormat = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDTimeFormat;
using FPDIndicator = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDIndicator;
using FPDState = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDState;
using FPDTextDisplay = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDTextDisplay;
using FPDMode = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDMode;

#define RDK_DSHAL_NAME "libds-hal.so"

#define ENTRY_LOG LOGINFO("%d: Enter %s \n", __LINE__, __func__);
#define EXIT_LOG LOGINFO("%d: EXIT %s \n", __LINE__, __func__);

static int fpd_isInitialized = 0;
static int fpd_isPlatInitialized = 0;

/** Structure that defines internal data base for the FP */
typedef struct _dsFPDSettings_t_
{   
    dsFPDBrightness_t brightness;
    dsFPDState_t state;
    dsFPDColor_t color;
}_FPDSettings_t;

static _FPDSettings_t srvFPDSettings[dsFPD_INDICATOR_MAX];

class dFPDImpl : public hal::dFPD::IPlatform {

    // delete copy constructor and assignment operator
    dFPDImpl(const dFPDImpl&) = delete;
    dFPDImpl& operator=(const dFPDImpl&) = delete;

public:
    dFPDImpl()
    {
        LOGINFO("dFPDImpl Constructor");
        InitialiseHAL();
    }

    virtual ~dFPDImpl()
    {
        LOGINFO("dFPDImpl Destructor");
        DeInitialiseHAL();
    }

    void InitialiseHAL()
    {
        LOGINFO("InitialiseHAL");
        if (!fpd_isInitialized) {
            for (int i = dsFPD_INDICATOR_MESSAGE; i < dsFPD_INDICATOR_MAX; i++)
            {
                srvFPDSettings[i].brightness = dsFPD_BRIGHTNESS_MAX;
                srvFPDSettings[i].state = dsFPD_STATE_OFF;
                            srvFPDSettings[i].color = dsFPD_COLOR_BLUE;
            }

            fpd_isInitialized = 1;

        }

        if (!fpd_isPlatInitialized) {
            LOGINFO("InitialiseHAL <dsFPD>");
            dsError_t eError = dsFPInit();
            if (dsERR_NONE != eError) {
                LOGERR("InitialiseHAL: dsFPInit failed with error: %d", eError);
                return;
            }
            LOGINFO("InitialiseHAL: dsFPInit succeeded");
            fpd_isPlatInitialized = 1;
        }
    }

    void DeInitialiseHAL()
    {
        LOGINFO("DeInitialiseHAL");
        if (fpd_isPlatInitialized)
        {
            dsFPTerm();
            fpd_isPlatInitialized = 0;
        }
        fpd_isInitialized = 0;
    }

    // Implementation of all FPD Platform interface methods
    uint32_t SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;

        LOGERR("SetFPDTime is DEPRECATED and not IMPLEMENTED");

        return retCode;
    }

    uint32_t SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;

        LOGERR("SetFPDScroll is DEPRECATED and not IMPLEMENTED");

        return retCode;
    }

    uint32_t SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;

        LOGERR("SetFPDBlink is DEPRECATED and not IMPLEMENTED");
        
        return retCode;
    }

    uint32_t SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("SetFPDBrightness: indicator %d, brightNess %d, persist %d", static_cast<int>(indicator), brightNess, persist);
        
        if (static_cast<int>(indicator) < dsFPD_INDICATOR_MAX && brightNess <= dsFPD_BRIGHTNESS_MAX) {
            dsError_t eError = dsSetFPBrightness(static_cast<dsFPDIndicator_t>(indicator), static_cast<dsFPDBrightness_t>(brightNess));
            LOGINFO("SetFPDBrightness: dsSetFPBrightness returned %d", eError);
            if (eError == dsERR_NONE) {
                srvFPDSettings[static_cast<int>(indicator)].brightness = brightNess;
                retCode = WPEFramework::Core::ERROR_NONE;
            } else {
                LOGERR("SetFPDBrightness: dsSetFPBrightness failed with error %d", eError);
            }
        } else {
            LOGERR("SetFPDBrightness: Invalid parameters - indicator %d, brightness %d", static_cast<int>(indicator), brightNess);
        }
        return retCode;
    }

    uint32_t GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("GetFPDBrightness: indicator %d", static_cast<int>(indicator));
        
        if (static_cast<int>(indicator) < dsFPD_INDICATOR_MAX) {
            dsFPDBrightness_t halBrightness = 0;
            dsError_t eError = dsGetFPBrightness(static_cast<dsFPDIndicator_t>(indicator), &halBrightness);
            LOGINFO("GetFPDBrightness: dsGetFPBrightness returned %d", eError);
            if (eError == dsERR_NONE) {
                brightNess = static_cast<uint32_t>(halBrightness);
                srvFPDSettings[static_cast<int>(indicator)].brightness = brightNess;
                retCode = WPEFramework::Core::ERROR_NONE;
            } else {
                LOGERR("GetFPDBrightness: dsGetFPBrightness failed with error %d", eError);
                // Fallback to cached value
                brightNess = srvFPDSettings[static_cast<int>(indicator)].brightness;
                retCode = WPEFramework::Core::ERROR_NONE;
            }
        } else {
            LOGERR("GetFPDBrightness: Invalid indicator %d", static_cast<int>(indicator));
        }
        return retCode;
    }

    uint32_t SetFPDState(const FPDIndicator indicator, const FPDState state) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("SetFPDState: indicator %d, state %d", static_cast<int>(indicator), static_cast<int>(state));
        
        if (static_cast<int>(indicator) < dsFPD_INDICATOR_MAX) {
            dsError_t eError = dsSetFPState(static_cast<dsFPDIndicator_t>(indicator), static_cast<dsFPDState_t>(state));
            LOGINFO("SetFPDState: dsSetFPState returned %d", eError);
            if (eError == dsERR_NONE) {
                srvFPDSettings[static_cast<int>(indicator)].state = static_cast<dsFPDState_t>(state);
                retCode = WPEFramework::Core::ERROR_NONE;
            } else {
                LOGERR("SetFPDState: dsSetFPState failed with error %d", eError);
            }
        } else {
            LOGERR("SetFPDState: Invalid indicator %d", static_cast<int>(indicator));
        }
        return retCode;
    }

    uint32_t GetFPDState(const FPDIndicator indicator, FPDState &state) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("GetFPDState: indicator %d", static_cast<int>(indicator));
        
        if (static_cast<int>(indicator) < dsFPD_INDICATOR_MAX) {
            dsFPDState_t halState = dsFPD_STATE_OFF;
            dsError_t eError = dsGetFPState(static_cast<dsFPDIndicator_t>(indicator), &halState);
            LOGINFO("GetFPDState: dsGetFPState returned %d", eError);
            if (eError == dsERR_NONE) {
                state = static_cast<FPDState>(halState);
                srvFPDSettings[static_cast<int>(indicator)].state = halState;
                retCode = WPEFramework::Core::ERROR_NONE;
            } else {
                LOGERR("GetFPDState: dsGetFPState failed with error %d", eError);
                // Fallback to cached value
                state = static_cast<FPDState>(srvFPDSettings[static_cast<int>(indicator)].state);
                retCode = WPEFramework::Core::ERROR_NONE;
            }
        } else {
            LOGERR("GetFPDState: Invalid indicator %d", static_cast<int>(indicator));
        }
        return retCode;
    }

    uint32_t GetFPDColor(const FPDIndicator indicator, uint32_t &color) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("GetFPDColor: indicator %d", static_cast<int>(indicator));
        
        if (static_cast<int>(indicator) < dsFPD_INDICATOR_MAX) {
            dsFPDColor_t halColor = 0;
            dsError_t eError = dsGetFPColor(static_cast<dsFPDIndicator_t>(indicator), &halColor);
            LOGINFO("GetFPDColor: dsGetFPColor returned %d", eError);
            if (eError == dsERR_NONE) {
                color = static_cast<uint32_t>(halColor);
                srvFPDSettings[static_cast<int>(indicator)].color = halColor;
                retCode = WPEFramework::Core::ERROR_NONE;
            } else {
                LOGERR("GetFPDColor: dsGetFPColor failed with error %d", eError);
                // Fallback to cached value
                color = srvFPDSettings[static_cast<int>(indicator)].color;
                retCode = WPEFramework::Core::ERROR_NONE;
            }
        } else {
            LOGERR("GetFPDColor: Invalid indicator %d", static_cast<int>(indicator));
        }
        return retCode;
    }

    uint32_t SetFPDColor(const FPDIndicator indicator, const uint32_t color) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("SetFPDColor: indicator %d, color %d", static_cast<int>(indicator), color);
        
        if (static_cast<int>(indicator) < dsFPD_INDICATOR_MAX && dsFPDColor_isValid(color)) {
            dsError_t eError = dsSetFPColor(static_cast<dsFPDIndicator_t>(indicator), static_cast<dsFPDColor_t>(color));
            LOGINFO("SetFPDColor: dsSetFPColor returned %d", eError);
            if (eError == dsERR_NONE) {
                srvFPDSettings[static_cast<int>(indicator)].color = static_cast<dsFPDColor_t>(color);
                retCode = WPEFramework::Core::ERROR_NONE;
            } else {
                LOGERR("SetFPDColor: dsSetFPColor failed with error %d", eError);
            }
        } else {
            LOGERR("SetFPDColor: Invalid parameters - indicator %d, color 0x%x", static_cast<int>(indicator), color);
        }
        return retCode;
    }

    uint32_t SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;

        LOGINFO("SetFPDTextBrightness is DEPRECATED and not IMPLEMENTED");

        return retCode;
    }

    uint32_t GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;

        LOGINFO("GetFPDTextBrightness is DEPRECATED and not IMPLEMENTED");

        return retCode;
    }

    uint32_t EnableFPDClockDisplay(const bool enable) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;

        LOGINFO("EnableFPDClockDisplay is DEPRECATED and not IMPLEMENTED");

        return retCode;
    }

    uint32_t GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;

        LOGINFO("GetFPDTimeFormat is DEPRECATED and not IMPLEMENTED");

        return retCode;
    }

    uint32_t SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;

        LOGINFO("SetFPDTimeFormat is DEPRECATED and not IMPLEMENTED");

        return retCode;
    }

    uint32_t SetFPDMode(const FPDMode fpdMode) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("SetFPDMode: fpdMode %d", static_cast<int>(fpdMode));
        
        dsError_t eError = dsSetFPDMode(static_cast<dsFPDMode_t>(fpdMode));
        LOGINFO("SetFPDMode: dsSetFPDMode returned %d", eError);
        if (eError == dsERR_NONE) {
            retCode = WPEFramework::Core::ERROR_NONE;
        } else {
            LOGERR("SetFPDMode: dsSetFPDMode failed with error %d", eError);
        }
        return retCode;
    }

    private:
};
