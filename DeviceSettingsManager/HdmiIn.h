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

#include <interfaces/Ids.h>
#include <interfaces/IDeviceSettingsManager.h>
//#include "Settings.h"            // for Settings

/*#include "dsMgr.h"
#include "dsUtl.h"
#include "dsError.h"
#include "dsDisplay.h"
#include "dsRpc.h"
#include "dsHdmiIn.h"
#include "dsError.h"*/

#include "list.hpp"
#include "host.hpp"
#include "exception.hpp"
#include "manager.hpp"
#include "hostPersistence.hpp"

#include "hal/dHdmiInImpl.h"
#include "deviceUtils.h"

#define ENTRY_LOG LOGINFO("%d: Enter %s \n", __LINE__, __func__);
#define EXIT_LOG LOGINFO("%d: EXIT %s \n", __LINE__, __func__);

namespace WPEFramework {
namespace Core {
    struct IDispatch;
    struct IWorkerPool;
}
}

class HdmiIn {
    using HDMIInPort               = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort;
    using HDMIInSignalStatus       = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInSignalStatus;
    using HDMIVideoPortResolution  = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIVideoPortResolution;
    using HDMIInAviContentType     = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInAviContentType;
    using HDMIInVRRType            = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInVRRType;
    using HDMIInStatus             = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInStatus;
    using HDMIVideoPlaneType       = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIVideoPlaneType;
    using HDMIInVRRStatus          = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInVRRStatus;
    using HDMIInCapabilityVersion  = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInCapabilityVersion;
    using HDMIInEdidVersion        = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInEdidVersion;
    using HDMIInVideoZoom          = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInVideoZoom;
    using HDMIInVideoRectangle     = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInVideoRectangle;
    //using IHDMIInPortConnectionStatusIterator = RPC::IIteratorType<HDMIPortConnectionStatus, ID_DEVICESETTINGS_MANAGER_HDMIIN_PORTCONNECTION_ITERATOR>;
    using IPlatform = hal::dHdmiIn::IPlatform;
    using DefaultImpl = dHdmiInImpl;

public:
    class INotification {

        public:
            virtual ~INotification() = default;

            //virtual void onThermalTemperatureChanged(const ThermalTemperature cur_Thermal_Level,const ThermalTemperature new_Thermal_Level, const float current_Temp) = 0;
            //virtual void onDeepSleepForThermalChange() = 0;
    };
    // We do not allow this plugin to be copied !!
    HdmiIn();

    void init();

    uint32_t GetHDMIInNumbefOfInputs(int32_t &count);
    //uint32_t GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus);
    uint32_t SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType);
    uint32_t ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition);
    uint32_t SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode);
    //uint32_t GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList);
    uint32_t GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency);
    uint32_t GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus);
    uint32_t GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport);
    uint32_t SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport);
    uint32_t GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]);
    uint32_t GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]);
    uint32_t GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion);
    uint32_t SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion);
    uint32_t GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution);
    uint32_t GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion);
    uint32_t SetVRRSupport(const HDMIInPort port, const bool vrrSupport);
    uint32_t GetVRRSupport(const HDMIInPort port, bool &vrrSupport);
    uint32_t GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus);

    template <typename IMPL = DefaultImpl, typename... Args>
    static HdmiIn Create(INotification& parent, Args&&... args)
    {
        static_assert(std::is_base_of<IPlatform, IMPL>::value, "Impl must derive from hal::dHdmiIn::IPlatform");
        auto impl = std::shared_ptr<IMPL>(new IMPL(std::forward<Args>(args)...));
        ASSERT(impl != nullptr);
        return HdmiIn(parent, std::move(impl));
    }

private:
    HdmiIn (INotification& parent, std::shared_ptr<IPlatform> platform);

    inline IPlatform& platform()
    {
        ASSERT(nullptr != _platform);
        return *_platform;
    }

    inline IPlatform& platform() const
    {
        return *_platform;
    }

    INotification& _parent;

    std::shared_ptr<IPlatform> _platform;

    void OnHDMIInEventHotPlug(const HDMIInPort port, const bool isConnected);

};
