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

#include "UtilsIarm.h"
#include "UtilsLogging.h"

#include <interfaces/json/JDeviceSettingsManagerFPD.h>
#include <interfaces/IDeviceSettingsManager.h>

#include "fpd.h"
#include "dsMgr.h"
#include "dsUtl.h"
#include "dsError.h"
#include "dsDisplay.h"
#include "dsRpc.h"

namespace WPEFramework {
namespace Plugin {
    class DeviceSettingsManagerImp :   public Exchange::IDeviceSettingsManagerFPD
    {
    public:

        // We do not allow this plugin to be copied !!
        DeviceSettingsManagerImp();
        ~DeviceSettingsManagerImp();

        static DeviceSettingsManagerImp* instance(DeviceSettingsManagerImp* DeviceSettingsManagerImpl = nullptr);

        // We do not allow this plugin to be copied !!
        DeviceSettingsManagerImp(const DeviceSettingsManagerImp&)            = delete;
        DeviceSettingsManagerImp& operator=(const DeviceSettingsManagerImp&) = delete;

        BEGIN_INTERFACE_MAP(DeviceSettingsManagerImp)
        INTERFACE_ENTRY(Exchange::IDeviceSettingsManagerFPD)
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

        Core::hresult Register(Exchange::IDeviceSettingsManagerFPD::INotification* notification) override;
        Core::hresult Unregister(Exchange::IDeviceSettingsManagerFPD::INotification* notification) override;
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

        static DeviceSettingsManagerImp* _instance;

    private:
        std::list<Exchange::IDeviceSettingsManagerFPD::INotification*> _FPDNotifications;

        // lock to guard all apis of PowerManager
        mutable Core::CriticalSection _apiLock;
        // lock to guard all notification from PowerManager to clients and also their callback register & unregister
        mutable Core::CriticalSection _callbackLock;

        template <typename T>
        Core::hresult Register(std::list<T*>& list, T* notification);
        template <typename T>
        Core::hresult Unregister(std::list<T*>& list, const T* notification);

        FPD _fpd;
    };
} // namespace Plugin
} // namespace WPEFramework
