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


#include <functional>
#include <interfaces/IDeviceSettingsManager.h>

struct CallbackBundle {
    std::function<void(WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInPort, bool)> OnHDMIInHotPlugEvent;
    std::function<void(WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInPort, WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInSignalStatus)> OnHDMIInSignalStatusEvent;
    std::function<void(WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInPort, bool)> OnHDMIInStatusEvent;
    std::function<void(WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInPort, WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIVideoPortResolution)> OnHDMIInVideoModeUpdateEvent;
    std::function<void(WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInPort, bool)> OnHDMIInAllmStatusEvent;
    std::function<void(WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInPort, WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInAviContentType)> OnHDMIInAVIContentTypeEvent;
    std::function<void(int32_t, int32_t)> OnHDMIInAVLatencyEvent;
    std::function<void(WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInPort, WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInVRRType)> OnHDMIInVRRStatusEvent;
    // Add other callbacks as needed
};
