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

#include "FirmwareVersion.h"
#include "mfrMgr.h"
#include "mfrApi.h"

#include <fstream>
#include <regex>

namespace WPEFramework {
namespace Plugin {
    namespace {
        uint32_t GetFileRegex(const char* filename, const std::regex& regex, string& response)
        {
            uint32_t result = Core::ERROR_GENERAL;

            std::ifstream file(filename);
            if (file) {
                string line;
                while (std::getline(file, line)) {
                    std::smatch sm;
                    if ((std::regex_match(line, sm, regex)) && (sm.size() > 1)) {
                        response = sm[1];

                        result = Core::ERROR_NONE;
                        break;
                    }
                }
            }

            return result;
        }

        uint32_t GetMFRData(mfrSerializedType_t type, string& response)
        {
            uint32_t result = Core::ERROR_GENERAL;
            int retVal = -1;
            mfrSerializedData_t mfrSerializedData;
            
            retVal = mfrGetSerializedData(type, &mfrSerializedData);
            if ((mfrERR_NONE == retVal) && mfrSerializedData.bufLen) {
                response = mfrSerializedData.buf;
                if (mfrSerializedData.freeBuf) {
                    mfrSerializedData.freeBuf(mfrSerializedData.buf);
                }
                
                return Core::ERROR_NONE;
            }
            
            return result;
        }
    }

    SERVICE_REGISTRATION(FirmwareVersion, 1, 0);

    Core::hresult FirmwareVersion::Imagename(string& imagename) const
    {
         return GetFileRegex(_T("/version.txt"), std::regex("^imagename:([^\\n]+)$"), imagename);
    }

    Core::hresult FirmwareVersion::Pdri(string& pdri) const
    {
        return GetMFRData(mfrSERIALIZED_TYPE_PDRIVERSION, pdri);
    }

    Core::hresult FirmwareVersion::Sdk(string& sdk) const
    {
        return GetFileRegex(_T("/version.txt"), std::regex("^SDK_VERSION=([^\\n]+)$"), sdk);
    }

    Core::hresult FirmwareVersion::Mediarite(string& mediarite) const
    {
        return GetFileRegex(_T("/version.txt"), std::regex("^MEDIARITE=([^\\n]+)$"), mediarite);
    }

    Core::hresult FirmwareVersion::Yocto(string& yocto) const
    {
        return GetFileRegex(_T("/version.txt"), std::regex("^YOCTO_VERSION=([^\\n]+)$"), yocto);
    }
}
}
