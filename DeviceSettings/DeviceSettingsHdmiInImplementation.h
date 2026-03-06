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

//#include <interfaces/IDeviceSettingsManager.h>
#include <interfaces/IDeviceSettingsAudio.h>
#include <interfaces/IDeviceSettingsCompositeIn.h>
#include <interfaces/IDeviceSettingsDisplay.h>
#include <interfaces/IDeviceSettingsFPD.h>
#include <interfaces/IDeviceSettingsHDMIIn.h>
#include <interfaces/IDeviceSettingsHost.h>
#include <interfaces/IDeviceSettingsVideoDevice.h>
#include <interfaces/IDeviceSettingsVideoPort.h>


#include "fpd.h"
#include "HdmiIn.h"

#include "list.hpp"
#include "DeviceSettingsTypes.h"

namespace WPEFramework {
namespace Plugin {
    class DeviceSettingsHdmiInImp :
                                   public Exchange::IDeviceSettingsHDMIIn
                                   , public HdmiIn::INotification
    {
    public:
        // Minimal implementations to satisfy IReferenceCounted
        uint32_t AddRef() const override { return 1; }
        uint32_t Release() const override { return 1; }

        // We do not allow this plugin to be copied !!
        DeviceSettingsHdmiInImp();
        ~DeviceSettingsHdmiInImp() override;

        static DeviceSettingsHdmiInImp* instance(DeviceSettingsHdmiInImp* DeviceSettingsImpl = nullptr);

        // We do not allow this plugin to be copied !!
        DeviceSettingsHdmiInImp(const DeviceSettingsHdmiInImp&)            = delete;
        DeviceSettingsHdmiInImp& operator=(const DeviceSettingsHdmiInImp&) = delete;

        BEGIN_INTERFACE_MAP(DeviceSettingsHdmiInImp)
        INTERFACE_ENTRY(Exchange::IDeviceSettingsHDMIIn)
        END_INTERFACE_MAP

    public: 
        class EXTERNAL LambdaJob : public Core::IDispatch {
        protected:
            LambdaJob(DeviceSettingsHdmiInImp* impl, std::function<void()> lambda)
                : _impl(impl)
                , _lambda(std::move(lambda))
            {
            }

        public:
            LambdaJob()                            = delete;
            LambdaJob(const LambdaJob&)            = delete;
            LambdaJob& operator=(const LambdaJob&) = delete;
            ~LambdaJob() {}

            static Core::ProxyType<Core::IDispatch> Create(DeviceSettingsHdmiInImp* impl, std::function<void()> lambda)
            {
                return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<LambdaJob>::Create(impl, std::move(lambda))));
            }

            virtual void Dispatch()
            {
                _lambda();
            }

        private:
            DeviceSettingsHdmiInImp* _impl;
            std::function<void()> _lambda;
        };

    public:
        void DeviceManager_Init();
        void InitializeIARM();

        // HDMIIn methods
        virtual Core::hresult Register(DeviceSettingsHDMIIn::INotification* notification) override;
        virtual Core::hresult Unregister(DeviceSettingsHDMIIn::INotification* notification) override;
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

        static DeviceSettingsHdmiInImp* _instance;

        
    private:
        std::list<DeviceSettingsHDMIIn::INotification*> _HDMIInNotifications;

        // lock to guard all apis of DeviceSettings
        mutable Core::CriticalSection _apiLock;
        // lock to guard all notification from DeviceSettings to clients and also their callback register & unregister
        mutable Core::CriticalSection _callbackLock;

        template <typename T>
        Core::hresult Register(std::list<T*>& list, T* notification);
        template <typename T>
        Core::hresult Unregister(std::list<T*>& list, const T* notification);

        template<typename Func, typename... Args>
        void dispatchHDMIInEvent(Func notifyFunc, Args&&... args);

        virtual void OnHDMIInEventHotPlugNotification(const HDMIInPort port, const bool isConnected) override;
        virtual void OnHDMIInEventSignalStatusNotification(const HDMIInPort port, const HDMIInSignalStatus signalStatus) override;
        virtual void OnHDMIInEventStatusNotification(const HDMIInPort activePort, const bool isPresented) override;
        virtual void OnHDMIInVideoModeUpdateNotification(const HDMIInPort port, const HDMIVideoPortResolution videoPortResolution) override;
        virtual void OnHDMIInAllmStatusNotification(const HDMIInPort port, const bool allmStatus) override;
        virtual void OnHDMIInAVIContentTypeNotification(const HDMIInPort port, const HDMIInAviContentType aviContentType) override;
        virtual void OnHDMIInAVLatencyNotification(const int32_t audioDelay, const int32_t videoDelay) override;
        virtual void OnHDMIInVRRStatusNotification(const HDMIInPort port, const HDMIInVRRType vrrType) override;

        HdmiIn _hdmiIn;
    };
} // namespace Plugin
} // namespace WPEFramework
