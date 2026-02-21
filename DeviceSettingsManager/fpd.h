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

#include "hal/dFPDImpl.h"

#define ENTRY_LOG LOGINFO("%d: Enter %s \n", __LINE__, __func__);
#define EXIT_LOG LOGINFO("%d: EXIT %s \n", __LINE__, __func__);

/*namespace WPEFramework {
namespace Core {
    struct IDispatch;
    struct IWorkerPool;
}
}*/

class FPD {
    using FPDTimeFormat = WPEFramework::Exchange::IDeviceSettingsManagerFPD::FPDTimeFormat;
    using FPDIndicator = WPEFramework::Exchange::IDeviceSettingsManagerFPD::FPDIndicator;
    using FPDState = WPEFramework::Exchange::IDeviceSettingsManagerFPD::FPDState;
    using FPDTextDisplay = WPEFramework::Exchange::IDeviceSettingsManagerFPD::FPDTextDisplay;
    using FPDMode = WPEFramework::Exchange::IDeviceSettingsManagerFPD::FPDMode;
    using FDPLEDState = WPEFramework::Exchange::IDeviceSettingsManagerFPD::FDPLEDState;

    using IPlatform = hal::dFPD::IPlatform;
    using DefaultImpl = dFPDImpl;

    std::shared_ptr<IPlatform> _platform;

public:
    class INotification {

        public:
            virtual ~INotification() = default;
            virtual void OnFPDTimeFormatChanged(const FPDTimeFormat timeFormat) = 0;
    };

public:

    // We do not allow this plugin to be copied !!
    //FPD();

    // Avoid copying this obj
    //FPD(const FPD&) = delete;            // copy constructor
    //FPD& operator=(const FPD&) = delete; // copy assignment operator

    void Platform_init();

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

    template <typename IMPL = DefaultImpl, typename... Args>
    static FPD Create(INotification& parent, Args&&... args)
    {
        ENTRY_LOG;
        static_assert(std::is_base_of<IPlatform, IMPL>::value, "Impl must derive from hal::dFPD::IPlatform");
        auto impl = std::shared_ptr<IMPL>(new IMPL(std::forward<Args>(args)...));
        ASSERT(impl != nullptr);
        EXIT_LOG;
        return FPD(parent, std::move(impl));
    }
    
    private:
    FPD(INotification& parent, std::shared_ptr<IPlatform> platform);

    inline IPlatform& platform() const
    {
        return *_platform;
    }

    INotification& _parent;

    //std::unique_ptr<IPlatform> _platform;
    //Settings _settings;
    //WPEFramework::Core::IWorkerPool& _workerPool;
};
