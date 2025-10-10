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
    //dHdmiInImpl(const dHdmiInImpl&) = delete;
    //dHdmiInImpl& operator=(const dHdmiInImpl&) = delete;

public:
    dFPDImpl()
    {
        LOGINFO("dFPDImpl Constructor");
        /*LOGINFO("HDMI version: %s", HdmiConnectionToStrMapping[0].name);
        LOGINFO("HDMI version: %s", HdmiStatusToStrMapping[0].name);
        LOGINFO("HDMI version: %s", HdmiVerToStrMapping[0].name);*/
        InitialiseHAL();
        // Initialize the platform
        /*pmStatus_t result = PLAT_INIT();
        if (PWRMGR_SUCCESS != result) {
            LOGERR("Failed to initialize power manager: %s", str(result));
        }*/
    }

    virtual ~dFPDImpl()
    {
        LOGINFO("dFPDImpl Destructor");
        DeInitialiseHAL();
        // Terminate the platform
        /*pmStatus_t result = PLAT_TERM();
        if (PWRMGR_SUCCESS != result) {
            LOGERR("Failed to terminate power manager: %s", str(result));
        }*/
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

    /*static void* resolve(const std::string& libName, const std::string& symbolName) {
        void* handle = dlopen(libName.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "dlopen failed for " << libName << ": " << dlerror() << std::endl;
            return nullptr;
        }
        void* symbol = dlsym(handle, symbolName.c_str());
        if (!symbol) {
            std::cerr << "dlsym failed for " << symbolName << ": " << dlerror() << std::endl;
        }
        dlclose(handle);
        return symbol;
    }*/

    // Implementation of all FPD Platform interface methods
    uint32_t SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("SetFPDTime: timeFormat %d, minutes %d, seconds %d", static_cast<int>(timeFormat), minutes, seconds);
        
        // Convert minutes/seconds to hour:minute format for HAL
        uint32_t hours = minutes / 60;
        uint32_t mins = minutes % 60;
        
        // First set the time format
        dsError_t eError = dsSetFPTimeFormat(static_cast<dsFPDTimeFormat_t>(timeFormat));
        if (eError != dsERR_NONE) {
            LOGERR("SetFPDTime: dsSetFPTimeFormat failed with error %d", eError);
            return retCode;
        }
        
        // Then set the actual time using hours and minutes
        eError = dsSetFPTime(static_cast<dsFPDTimeFormat_t>(timeFormat), hours, mins);
        LOGINFO("SetFPDTime: dsSetFPTime returned %d", eError);
        if (eError == dsERR_NONE) {
            retCode = WPEFramework::Core::ERROR_NONE;
        } else {
            LOGERR("SetFPDTime: dsSetFPTime failed with error %d", eError);
        }
        return retCode;
    }

    uint32_t SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("SetFPDScroll: scrollHoldDuration %d, nHorizontalScrollIterations %d, nVerticalScrollIterations %d", scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations);
        
        dsError_t eError = dsSetFPScroll(scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations);
        LOGINFO("SetFPDScroll: dsSetFPScroll returned %d", eError);
        if (eError == dsERR_NONE) {
            retCode = WPEFramework::Core::ERROR_NONE;
        } else {
            LOGERR("SetFPDScroll: dsSetFPScroll failed with error %d", eError);
        }
        return retCode;
    }

    uint32_t SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("SetFPDBlink: indicator %d, blinkDuration %d, blinkIterations %d", static_cast<int>(indicator), blinkDuration, blinkIterations);
        
        if (static_cast<int>(indicator) < dsFPD_INDICATOR_MAX) {
            dsError_t eError = dsSetFPBlink(static_cast<dsFPDIndicator_t>(indicator), blinkDuration, blinkIterations);
            LOGINFO("SetFPDBlink: dsSetFPBlink returned %d", eError);
            if (eError == dsERR_NONE) {
                retCode = WPEFramework::Core::ERROR_NONE;
            } else {
                LOGERR("SetFPDBlink: dsSetFPBlink failed with error %d", eError);
            }
        } else {
            LOGERR("SetFPDBlink: Invalid indicator %d", static_cast<int>(indicator));
        }
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
        LOGINFO("SetFPDTextBrightness: textDisplay %d, brightNess %d", static_cast<int>(textDisplay), brightNess);
        
        if (static_cast<int>(textDisplay) < dsFPD_TEXTDISP_MAX && brightNess <= dsFPD_BRIGHTNESS_MAX) {
            dsError_t eError = dsSetFPTextBrightness(static_cast<dsFPDTextDisplay_t>(textDisplay), static_cast<dsFPDBrightness_t>(brightNess));
            LOGINFO("SetFPDTextBrightness: dsSetFPTextBrightness returned %d", eError);
            if (eError == dsERR_NONE) {
                retCode = WPEFramework::Core::ERROR_NONE;
            } else {
                LOGERR("SetFPDTextBrightness: dsSetFPTextBrightness failed with error %d", eError);
            }
        } else {
            LOGERR("SetFPDTextBrightness: Invalid parameters - textDisplay %d, brightness %d", static_cast<int>(textDisplay), brightNess);
        }
        return retCode;
    }

    uint32_t GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("GetFPDTextBrightness: textDisplay %d", static_cast<int>(textDisplay));
        
        if (static_cast<int>(textDisplay) < dsFPD_TEXTDISP_MAX) {
            dsFPDBrightness_t halBrightness = 0;
            dsError_t eError = dsGetFPTextBrightness(static_cast<dsFPDTextDisplay_t>(textDisplay), &halBrightness);
            LOGINFO("GetFPDTextBrightness: dsGetFPTextBrightness returned %d", eError);
            if (eError == dsERR_NONE) {
                brightNess = static_cast<uint32_t>(halBrightness);
                retCode = WPEFramework::Core::ERROR_NONE;
            } else {
                LOGERR("GetFPDTextBrightness: dsGetFPTextBrightness failed with error %d", eError);
                brightNess = 100; // Default fallback value
                retCode = WPEFramework::Core::ERROR_NONE;
            }
        } else {
            LOGERR("GetFPDTextBrightness: Invalid textDisplay %d", static_cast<int>(textDisplay));
            brightNess = 100; // Default fallback value
        }
        return retCode;
    }

    uint32_t EnableFPDClockDisplay(const bool enable) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("EnableFPDClockDisplay: enable %d", enable);
        
        dsError_t eError = dsFPEnableCLockDisplay(enable);
        LOGINFO("EnableFPDClockDisplay: dsFPEnableCLockDisplay returned %d", eError);
        if (eError == dsERR_NONE) {
            retCode = WPEFramework::Core::ERROR_NONE;
        } else {
            LOGERR("EnableFPDClockDisplay: dsFPEnableCLockDisplay failed with error %d", eError);
        }
        return retCode;
    }

    uint32_t GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("GetFPDTimeFormat");
        
        dsFPDTimeFormat_t halTimeFormat = dsFPD_TIME_12_HOUR;
        dsError_t eError = dsGetFPTimeFormat(&halTimeFormat);
        LOGINFO("GetFPDTimeFormat: dsGetFPTimeFormat returned %d", eError);
        if (eError == dsERR_NONE) {
            fpdTimeFormat = static_cast<FPDTimeFormat>(halTimeFormat);
            retCode = WPEFramework::Core::ERROR_NONE;
        } else {
            LOGERR("GetFPDTimeFormat: dsGetFPTimeFormat failed with error %d", eError);
            fpdTimeFormat = static_cast<FPDTimeFormat>(dsFPD_TIME_12_HOUR); // Default fallback
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        LOGINFO("SetFPDTimeFormat: fpdTimeFormat %d", static_cast<int>(fpdTimeFormat));
        
        dsError_t eError = dsSetFPTimeFormat(static_cast<dsFPDTimeFormat_t>(fpdTimeFormat));
        LOGINFO("SetFPDTimeFormat: dsSetFPTimeFormat returned %d", eError);
        if (eError == dsERR_NONE) {
            retCode = WPEFramework::Core::ERROR_NONE;
        } else {
            LOGERR("SetFPDTimeFormat: dsSetFPTimeFormat failed with error %d", eError);
        }
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
