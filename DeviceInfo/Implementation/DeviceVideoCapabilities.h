/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
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
**/

#pragma once

#include "../Module.h"
#ifdef USE_THUNDER_R4
#include <interfaces/IDeviceInfo.h>
#else
#include <interfaces/IDeviceInfo2.h>
#endif /* USE_THUNDER_R4 */

namespace WPEFramework {
namespace Plugin {
    class DeviceVideoCapabilities : public Exchange::IDeviceVideoCapabilities {
    private:
        DeviceVideoCapabilities(const DeviceVideoCapabilities&) = delete;
        DeviceVideoCapabilities& operator=(const DeviceVideoCapabilities&) = delete;

    public:
        DeviceVideoCapabilities();

        BEGIN_INTERFACE_MAP(DeviceVideoCapabilities)
        INTERFACE_ENTRY(Exchange::IDeviceVideoCapabilities)
        END_INTERFACE_MAP

    private:
        // IDeviceVideoCapabilities interface
        uint32_t SupportedVideoDisplays(RPC::IStringIterator*& supportedVideoDisplays) const override;
        uint32_t HostEDID(string& edid) const override;
        uint32_t DefaultResolution(const string& videoDisplay, string& defaultResolution) const override;
        uint32_t SupportedResolutions(const string& videoDisplay, RPC::IStringIterator*& supportedResolutions) const override;
        uint32_t SupportedHdcp(const string& videoDisplay, Exchange::IDeviceVideoCapabilities::CopyProtection& supportedHDCPVersion) const override;
    };
}
}
