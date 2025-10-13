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

#include <cstdint>     // for uint32_t
#include <memory>      // for unique_ptr, default_delete
#include <string>      // for basic_string, string
#include <type_traits> // for is_base_of
#include <utility>     // for forward

#include <core/Portability.h>         // for string, ErrorCodes
#include <core/Proxy.h>               // for ProxyType
#include <core/Trace.h>               // for ASSERT

#include "UtilsLogging.h"
#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#include <interfaces/IDeviceSettingsManager.h>
//#include "Settings.h"            // for Settings

#include "dsMgr.h"
#include "dsUtl.h"
#include "dsError.h"
#include "dsDisplay.h"
#include "dsRpc.h"
#include "dsFPDTypes.h"

#define ENTRY_LOG LOGINFO("%d: Enter %s \n", __LINE__, __func__);
#define EXIT_LOG LOGINFO("%d: EXIT %s \n", __LINE__, __func__);

/*namespace WPEFramework {
namespace Core {
    struct IDispatch;
    struct IWorkerPool;
}
}*/

class FPD {
    using FPDTimeFormat = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDTimeFormat;
    using FPDIndicator = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDIndicator;
    using FPDState = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDState;
    using FPDTextDisplay = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDTextDisplay;
    using FPDMode = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDMode;
    using FDPLEDState = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FDPLEDState;

    /*inline IPlatform& platform()
    {
        ASSERT(nullptr != _platform);
        return *_platform;
    }

    inline IPlatform& platform() const
    {
        return *_platform;
    }*/

public:

    // We do not allow this plugin to be copied !!
    FPD();

    // Avoid copying this obj
    FPD(const FPD&) = delete;            // copy constructor
    FPD& operator=(const FPD&) = delete; // copy assignment operator

    void init();

    uint32_t SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds);
    uint32_t SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations);
    uint32_t SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations);
    uint32_t SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist);
    uint32_t GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess);
    uint32_t SetFPDState(const FPDIndicator indicator, const FPDState state);
    uint32_t GetFPDState(const FPDIndicator indicator, FPDState &state);
    uint32_t GetFPDColor(const FPDIndicator indicator, uint32_t &color);
    uint32_t SetFPDColor(const FPDIndicator indicator, const uint32_t color);
    uint32_t SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess);
    uint32_t GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess);
    uint32_t EnableFPDClockDisplay(const bool enable);
    uint32_t GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat);
    uint32_t SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat);
    uint32_t SetFPDMode(const FPDMode fpdMode);

    #define dsFPDColor_Make(R8,G8,B8)  (((R8)<<16) | ((G8)<< 8) | ((B8) ))
    #define dsFPDColor_R(RGB32)    (((RGB32) >> 16) & 0xFF)                ///< Extract Red value form RGB value
    #define dsFPDColor_G(RGB32)    (((RGB32) >>  8) & 0xFF)                ///< Extract Green value form RGB value
    #define dsFPDColor_B(RGB32)    (((RGB32)      ) & 0xFF)                ///< Extract Blue value form RGB value

    #define dsFPD_COLOR_BLUE   dsFPDColor_Make(0, 0, 0xFF)          ///< Blue color LED                 
    #define dsFPD_COLOR_GREEN  dsFPDColor_Make(0, 0xFF, 0)          ///< Green color LED                
    #define dsFPD_COLOR_RED    dsFPDColor_Make(0xFF, 0, 0x0)        ///< Red color LED                 
    #define dsFPD_COLOR_YELLOW dsFPDColor_Make(0xFF, 0xFF, 0xE0)    ///< Yellow color LED               
    #define dsFPD_COLOR_ORANGE dsFPDColor_Make(0xFF, 0x8C, 0x00)    ///< Orange color LED               
    #define dsFPD_COLOR_WHITE  dsFPDColor_Make(0xFF, 0xFF, 0xFF)    ///< White color LED               
    #define dsFPD_COLOR_MAX    6                                    ///< Out of range

    /*template <typename IMPL = DefaultImpl, typename... Args>
    static PowerController Create(DeepSleepController& deepSleep, Args&&... args)
    {
        static_assert(std::is_base_of<IPlatform, IMPL>::value, "Impl must derive from hal::power::IPlatform");
        IMPL* api = new IMPL(std::forward<Args>(args)...);
        return PowerController(deepSleep, std::unique_ptr<IPlatform>(api));
    }*/

private:
    //std::unique_ptr<IPlatform> _platform;
    //Settings _settings;
    //WPEFramework::Core::IWorkerPool& _workerPool;
};
