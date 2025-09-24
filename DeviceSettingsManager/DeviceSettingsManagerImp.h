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
#include <core/Portability.h>
#include <cstdint> // Ensure this is present for uint32_t

#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#include <interfaces/IDeviceSettingsManager.h>

#include "fpd.h"
#include "HdmiIn.h"

#include "list.hpp"

#define ENTRY_LOG LOGINFO("%d: Enter %s \n", __LINE__, __func__);
#define EXIT_LOG LOGINFO("%d: EXIT %s \n", __LINE__, __func__);

namespace WPEFramework {
namespace Plugin {
    class DeviceSettingsManagerImp : public Exchange::IDeviceSettingsManager::IFPD
                                   , public Exchange::IDeviceSettingsManager::IHDMIIn
                                   , public HdmiIn::INotification
    {
    public:

        // We do not allow this plugin to be copied !!
        DeviceSettingsManagerImp();
        ~DeviceSettingsManagerImp() override;

        static DeviceSettingsManagerImp* instance(DeviceSettingsManagerImp* DeviceSettingsManagerImpl = nullptr);

        // We do not allow this plugin to be copied !!
        DeviceSettingsManagerImp(const DeviceSettingsManagerImp&)            = delete;
        DeviceSettingsManagerImp& operator=(const DeviceSettingsManagerImp&) = delete;

        BEGIN_INTERFACE_MAP(DeviceSettingsManagerImp)
        INTERFACE_ENTRY(Exchange::IDeviceSettingsManager::IFPD)
        INTERFACE_ENTRY(Exchange::IDeviceSettingsManager::IHDMIIn)
        END_INTERFACE_MAP

    public:
        class EXTERNAL LambdaJob : public Core::IDispatch {
        protected:
            LambdaJob(DeviceSettingsManagerImp* impl, std::function<void()> lambda)
                : _impl(impl)
                , _lambda(std::move(lambda))
            {
                if (_impl != nullptr) {
                    _impl->AddRef();
                }
            }

        public:
            LambdaJob()                            = delete;
            LambdaJob(const LambdaJob&)            = delete;
            LambdaJob& operator=(const LambdaJob&) = delete;
            ~LambdaJob()
            {
                if (_impl != nullptr) {
                    _impl->Release();
                }
            }

            static Core::ProxyType<Core::IDispatch> Create(DeviceSettingsManagerImp* impl, std::function<void()> lambda)
            {
                return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<LambdaJob>::Create(impl, std::move(lambda))));
            }

            virtual void Dispatch()
            {
                _lambda();
            }

        private:
            DeviceSettingsManagerImp* _impl;
            std::function<void()> _lambda;
        };

    public:
        void DeviceManager_Init();
        void InitializeIARM();

        // FPD methods
        Core::hresult Register(Exchange::IDeviceSettingsManager::IFPD::INotification* notification) override;
        Core::hresult Unregister(Exchange::IDeviceSettingsManager::IFPD::INotification* notification) override;
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

        // HDMIIn methods
        Core::hresult Register(Exchange::IDeviceSettingsManager::IHDMIIn::INotification* notification) override;
        Core::hresult Unregister(Exchange::IDeviceSettingsManager::IHDMIIn::INotification* notification) override;
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

        static DeviceSettingsManagerImp* _instance;

    private:
        std::list<Exchange::IDeviceSettingsManager::IFPD::INotification*> _FPDNotifications;
        std::list<Exchange::IDeviceSettingsManager::IHDMIIn::INotification*> _HDMIInNotifications;

        // lock to guard all apis of DeviceSettingsManager
        mutable Core::CriticalSection _apiLock;
        // lock to guard all notification from DeviceSettingsManager to clients and also their callback register & unregister
        mutable Core::CriticalSection _callbackLock;

        template <typename T>
        Core::hresult Register(std::list<T*>& list, T* notification);
        template <typename T>
        Core::hresult Unregister(std::list<T*>& list, const T* notification);

        void dispatchHDMIInHotPlugEvent(const HDMIInPort port, const bool isConnected);
        FPD _fpd;
        HdmiIn _hdmiIn;
    };
} // namespace Plugin
} // namespace WPEFramework
