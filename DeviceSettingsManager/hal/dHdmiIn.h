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

#include "deviceUtils.h"
#include "dsHdmiIn.h"
#include "dsError.h"
#include "dsHdmiInTypes.h"
#include "dsUtl.h"
#include "dsTypes.h"
#include "dsInternal.h"

#include "rfcapi.h"

#include <core/Portability.h>
#include <interfaces/IDeviceSettingsManager.h>

#include "dHdmiIn.h"
#include "deviceUtils.h"
#include "UtilsLogging.h"
#include <cstdint>

#include <core/Portability.h>

namespace hal {
namespace dHdmiIn {

    // Power APIs
    class IPlatform {
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

    public:
        virtual ~IPlatform() {}
        void InitialiseHAL();
        virtual void setAllCallbacks(const CallbackBundle bundle) = 0;
        virtual uint32_t GetHDMIInNumbefOfInputs(int32_t &count) = 0;

        static std::function<void(WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort, bool)> g_HdmiInHotPlugCallback;
        static void DS_OnHDMIInEventHotPlug(const dsHdmiInPort_t port, const bool isConnected);
        //virtual uint32_t GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) = 0;
        /*virtual uint32_t SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) = 0;
        virtual uint32_t ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) = 0;
        virtual uint32_t SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) = 0;
        //virtual uint32_t GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) = 0;
        virtual uint32_t GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) = 0;
        virtual uint32_t GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) = 0;
        virtual uint32_t GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) = 0;
        virtual uint32_t SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) = 0;
        virtual uint32_t GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) = 0;
        virtual uint32_t GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) = 0;
        virtual uint32_t GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) = 0;
        virtual uint32_t SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) = 0;
        virtual uint32_t GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) = 0;
        virtual uint32_t GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) = 0;
        virtual uint32_t SetVRRSupport(const HDMIInPort port, const bool vrrSupport) = 0;
        virtual uint32_t GetVRRSupport(const HDMIInPort port, bool &vrrSupport) = 0;
        virtual uint32_t GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) = 0;*/
    };
} // namespace power
} // namespace hal

