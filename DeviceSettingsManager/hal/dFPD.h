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

#include "rfcapi.h"

#include <core/Portability.h>
#include <WPEFramework/interfaces/IDeviceSettingsManager.h>

#include "deviceUtils.h"
#include "UtilsLogging.h"
#include <cstdint>

#include <core/Portability.h>

using FPDTimeFormat = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDTimeFormat;
using FPDIndicator = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDIndicator;
using FPDState = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDState;
using FPDTextDisplay = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDTextDisplay;
using FPDMode = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDMode;
using FDPLEDState = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FDPLEDState;

namespace hal {
namespace dFPD {

    class IPlatform {

    // delete copy constructor and assignment operator
    //dHdmiInImpl(const dHdmiInImpl&) = delete;
    //dHdmiInImpl& operator=(const dHdmiInImpl&) = delete;

    public:
        virtual ~IPlatform() {}
        void InitialiseHAL();
        void DeInitialiseHAL();
        //virtual void setAllCallbacks(const CallbackBundle bundle) = 0;
        //virtual void getPersistenceValue() = 0;
        //virtual void deinit();

        virtual uint32_t SetFPDBrightness(const FPDIndicator indicator /* @in */, const uint32_t brightNess /* @in */, const bool persist /* @in */) = 0;

    };
} // namespace dFPD
} // namespace hal

