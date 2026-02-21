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
    class DeviceSettingsImp
    {
    public:
        // We do not allow this plugin to be copied !!
        DeviceSettingsImp();
        ~DeviceSettingsImp();

        static DeviceSettingsImp* instance(DeviceSettingsImp* DeviceSettingsImpl = nullptr);

        // We do not allow this plugin to be copied !!
        DeviceSettingsImp(const DeviceSettingsImp&)            = delete;
        DeviceSettingsImp& operator=(const DeviceSettingsImp&) = delete;

    public:
        class EXTERNAL LambdaJob : public Core::IDispatch {
        protected:
            LambdaJob(DeviceSettingsImp* impl, std::function<void()> lambda)
                : _impl(impl)
                , _lambda(std::move(lambda))
            {
            }

        public:
            LambdaJob()                            = delete;
            LambdaJob(const LambdaJob&)            = delete;
            LambdaJob& operator=(const LambdaJob&) = delete;
            ~LambdaJob() {}

            static Core::ProxyType<Core::IDispatch> Create(DeviceSettingsImp* impl, std::function<void()> lambda)
            {
                return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<LambdaJob>::Create(impl, std::move(lambda))));
            }

            virtual void Dispatch()
            {
                _lambda();
            }

        private:
            DeviceSettingsImp* _impl;
            std::function<void()> _lambda;
        };

    public:
        void DeviceManager_Init();
        void InitializeIARM();

        static DeviceSettingsImp* _instance;

        
    private:
        // lock to guard all apis of DeviceSettings
        mutable Core::CriticalSection _apiLock;
        // lock to guard all notification from DeviceSettings to clients and also their callback register & unregister
        mutable Core::CriticalSection _callbackLock;
    };
} // namespace Plugin
} // namespace WPEFramework
