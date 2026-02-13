/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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

#include "Module.h"

#include <memory>
#include <unordered_map>
#include <chrono>
#include <cstdint>

#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#include <interfaces/IDeviceSettings.h>
#include <interfaces/IDeviceSettingsAudio.h>
#include <interfaces/IDeviceSettingsCompositeIn.h>
#include <interfaces/IDeviceSettingsDisplay.h>
#include <interfaces/IDeviceSettingsFPD.h>
#include <interfaces/IDeviceSettingsHDMIIn.h>
#include <interfaces/IDeviceSettingsHost.h>
#include <interfaces/IDeviceSettingsVideoDevice.h>
#include <interfaces/IDeviceSettingsVideoPort.h>

// Forward declarations are no longer needed since we use QueryInterface
// to get interface handles from the WPEFramework service system


//#include "fpd.h"
//#include "HdmiIn.h"

#include "list.hpp"
#include "DeviceSettingsTypes.h"

namespace WPEFramework {
namespace Plugin {
    class DeviceSettingsImp : public Exchange::IDeviceSettings
                             , public Exchange::IDeviceSettingsFPD
                             , public Exchange::IDeviceSettingsHDMIIn
                             // , public Exchange::IDeviceSettingsAudio         // Not implemented yet
                             // , public Exchange::IDeviceSettingsCompositeIn   // Not implemented yet
                             // , public Exchange::IDeviceSettingsDisplay       // Not implemented yet
                             // , public Exchange::IDeviceSettingsHost          // Not implemented yet
                             // , public Exchange::IDeviceSettingsVideoDevice   // Not implemented yet
                             // , public Exchange::IDeviceSettingsVideoPort     // Not implemented yet
    {
    public:
        // We do not allow this plugin to be copied !!
        DeviceSettingsImp();
        ~DeviceSettingsImp();

        static DeviceSettingsImp* instance(DeviceSettingsImp* DeviceSettingsImpl = nullptr);

        // We do not allow this plugin to be copied !!
        DeviceSettingsImp(const DeviceSettingsImp&)            = delete;
        DeviceSettingsImp& operator=(const DeviceSettingsImp&) = delete;

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(DeviceSettingsImp)
            INTERFACE_ENTRY(Exchange::IDeviceSettings)
            INTERFACE_ENTRY(Exchange::IDeviceSettingsFPD)
            INTERFACE_ENTRY(Exchange::IDeviceSettingsHDMIIn)
            // Future interface entries when implemented:
            // INTERFACE_ENTRY(Exchange::IDeviceSettingsAudio)
            // INTERFACE_ENTRY(Exchange::IDeviceSettingsCompositeIn)
            // INTERFACE_ENTRY(Exchange::IDeviceSettingsDisplay)
            // INTERFACE_ENTRY(Exchange::IDeviceSettingsHost)
            // INTERFACE_ENTRY(Exchange::IDeviceSettingsVideoDevice)
            // INTERFACE_ENTRY(Exchange::IDeviceSettingsVideoPort)
        END_INTERFACE_MAP

        // IDeviceSettings interface implementation
        Core::hresult Configure(PluginHost::IShell* service) override;
        
        // IDeviceSettingsFPD interface implementation - delegate to _fpdSettings interface
        Core::hresult Register(Exchange::IDeviceSettingsFPD::INotification* notification) override;
        Core::hresult Unregister(Exchange::IDeviceSettingsFPD::INotification* notification) override;
        Core::hresult SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) override;
        Core::hresult SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) override;
        Core::hresult SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) override;
        Core::hresult SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) override;
        Core::hresult GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) override;
        Core::hresult SetFPDState(const FPDIndicator indicator, const FPDState state) override;
        Core::hresult GetFPDState(const FPDIndicator indicator, FPDState &state) override;
        Core::hresult GetFPDColor(const FPDIndicator indicator, uint32_t &color) override;
        Core::hresult SetFPDColor(const FPDIndicator indicator, const uint32_t color) override;
        Core::hresult SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) override;
        Core::hresult GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) override;
        Core::hresult EnableFPDClockDisplay(const bool enable) override;
        Core::hresult GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) override;
        Core::hresult SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) override;
        Core::hresult SetFPDMode(const FPDMode fpdMode) override;
        
        // IDeviceSettingsHDMIIn interface implementation - delegate to _hdmiInSettings interface
        Core::hresult Register(Exchange::IDeviceSettingsHDMIIn::INotification* notification) override;
        Core::hresult Unregister(Exchange::IDeviceSettingsHDMIIn::INotification* notification) override;
        Core::hresult GetHDMIInNumbefOfInputs(int32_t &count) override;
        Core::hresult GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) override;
        Core::hresult SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) override;
        Core::hresult ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) override;
        Core::hresult SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) override;
        Core::hresult GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) override;
        Core::hresult GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) override;
        Core::hresult GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) override;
        Core::hresult GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) override;
        Core::hresult SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) override;
        Core::hresult GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) override;
        Core::hresult GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) override;
        Core::hresult GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) override;
        Core::hresult SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) override;
        Core::hresult GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) override;
        Core::hresult GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) override;
        Core::hresult SetVRRSupport(const HDMIInPort port, const bool vrrSupport) override;
        Core::hresult GetVRRSupport(const HDMIInPort port, bool &vrrSupport) override;
        Core::hresult GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) override;
        
        // Other interface implementations - stub implementations for now
        // IDeviceSettingsAudio - not implemented yet
        // IDeviceSettingsCompositeIn - not implemented yet  
        // IDeviceSettingsDisplay - not implemented yet
        // IDeviceSettingsHost - not implemented yet
        // IDeviceSettingsVideoDevice - not implemented yet
        // IDeviceSettingsVideoPort - not implemented yet

    private:
        void InitializeComponentHandles(PluginHost::IShell* service);
        
        // Component interface handles (obtained via QueryInterface on this instance)
        // Only for interfaces we actually implement
        Exchange::IDeviceSettingsFPD* _fpdSettings;
        Exchange::IDeviceSettingsHDMIIn* _hdmiInSettings;
        
        // Interface pointers for future implementation (currently unused)
        // Exchange::IDeviceSettingsAudio* _audioSettings;
        // Exchange::IDeviceSettingsCompositeIn* _compositeInSettings;
        // Exchange::IDeviceSettingsDisplay* _displaySettings;
        // Exchange::IDeviceSettingsHost* _hostSettings;
        // Exchange::IDeviceSettingsVideoDevice* _videoDeviceSettings;
        // Exchange::IDeviceSettingsVideoPort* _videoPortSettings;
        
        uint32_t mConnectionId;
        static DeviceSettingsImp* _instance;
    };
} // namespace Plugin
} // namespace WPEFramework
