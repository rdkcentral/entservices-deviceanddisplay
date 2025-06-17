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
    class DeviceInfoImplementation : public Exchange::IDeviceInfo {
    private:
        DeviceInfoImplementation(const DeviceInfoImplementation&) = delete;
        DeviceInfoImplementation& operator=(const DeviceInfoImplementation&) = delete;

    public:
        DeviceInfoImplementation();

        BEGIN_INTERFACE_MAP(DeviceInfoImplementation)
        INTERFACE_ENTRY(Exchange::IDeviceInfo)
        END_INTERFACE_MAP

    private:
        // IDeviceInfo interface
        uint32_t SerialNumber(string& serialNumber) const override;
        uint32_t Sku(string& sku) const override;
        uint32_t Make(string& make) const override;
        uint32_t Model(string& model) const override;
        uint32_t DeviceType(string& deviceType) const override;
        uint32_t SocName(string& socName) const override;
        uint32_t DistributorId(string& distributorId) const override;
        uint32_t Brand(string& brand) const override;
        Core::hresult ReleaseVersion(string& releaseVersion ) const override;
        Core::hresult ChipSet(string& chipSet ) const override;
    };
}
}
