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
//#include <interfaces/IDeviceSettingsManager.h>
#include <interfaces/IDeviceSettingsAudio.h>
#include <interfaces/IDeviceSettingsCompositeIn.h>
#include <interfaces/IDeviceSettingsDisplay.h>
#include <interfaces/IDeviceSettingsFPD.h>
#include <interfaces/IDeviceSettingsHDMIIn.h>
#include <interfaces/IDeviceSettingsHost.h>
#include <interfaces/IDeviceSettingsVideoDevice.h>
#include <interfaces/IDeviceSettingsVideoPort.h>


//#include "fpd.h"
//#include "HdmiIn.h"

#include "list.hpp"
#include "DeviceSettingsTypes.h"

namespace WPEFramework {
namespace Plugin {
    class DeviceSettingsImp : public Exchange::IDeviceSettings
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
        END_INTERFACE_MAP

        // IDeviceSettings interface implementation
        Core::hresult Configure(PluginHost::IShell* service) override;

    private:
        void InitializeComponentHandles(PluginHost::IShell* service);
        
        // Component interface handles
        Exchange::IDeviceSettingsAudio* _audioSettings;
        Exchange::IDeviceSettingsCompositeIn* _compositeInSettings;
        Exchange::IDeviceSettingsDisplay* _displaySettings;
        Exchange::IDeviceSettingsFPD* _fpdSettings;
        Exchange::IDeviceSettingsHDMIIn* _hdmiInSettings;
        Exchange::IDeviceSettingsHost* _hostSettings;
        Exchange::IDeviceSettingsVideoDevice* _videoDeviceSettings;
        Exchange::IDeviceSettingsVideoPort* _videoPortSettings;
        
        uint32_t mConnectionId;
        static DeviceSettingsImp* _instance;
    };
} // namespace Plugin
} // namespace WPEFramework
