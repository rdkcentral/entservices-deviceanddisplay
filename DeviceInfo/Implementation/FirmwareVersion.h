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
#include <interfaces/IFirmwareVersion.h>

namespace WPEFramework {
namespace Plugin {
    class FirmwareVersion : public Exchange::IFirmwareVersion {
    private:
        FirmwareVersion(const FirmwareVersion&) = delete;
        FirmwareVersion& operator=(const FirmwareVersion&) = delete;

    public:
        FirmwareVersion() = default;

        BEGIN_INTERFACE_MAP(FirmwareVersion)
        INTERFACE_ENTRY(Exchange::IFirmwareVersion)
        END_INTERFACE_MAP

    private:
        // IFirmwareVersion interface
        Core::hresult Imagename(string& imagename) const override;
        Core::hresult Sdk(string& sdk) const override;
        Core::hresult Mediarite(string& mediarite) const override;
        Core::hresult Yocto(string& yocto) const override;
       // Core::hresult Pdri(string& pdri) const override;
    };
}
}
