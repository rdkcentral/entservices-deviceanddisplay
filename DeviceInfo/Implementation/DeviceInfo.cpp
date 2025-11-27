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

#include "DeviceInfo.h"

#include "mfrMgr.h"
#include "rfcapi.h"

#include "UtilsIarm.h"

#include <fstream>
#include <regex>

namespace WPEFramework {
namespace Plugin {
    namespace {

        uint32_t GetFileRegex(const char* filename, const std::regex& regex, string& response)
        {
            uint32_t result = Core::ERROR_GENERAL;

            // FIX(Coverity): Resource Leak - Add explicit file open check
            // Reason: Verify file was successfully opened before attempting operations
            // Impact: No API signature changes. Better error handling for file operations.
            std::ifstream file(filename);
            if (file.is_open() && file.good()) {
                string line;
                while (std::getline(file, line)) {
                    std::smatch sm;
                    if (std::regex_match(line, sm, regex)) {
                        // FIX(Coverity): Logic Error - Replace ASSERT with runtime check
                        // Reason: ASSERT is compiled out in release builds
                        // Impact: No API signature changes. Proper runtime validation of regex matches.
                        if (sm.size() >= 2) {
                            response = sm[1];
                            result = Core::ERROR_NONE;
                        } else {
                            TRACE_GLOBAL(Trace::Warning, (_T("Regex match found but capture group size is %zu, expected >= 2"), sm.size()));
                        }
                        break;
                    }
                }
            } else {
                TRACE_GLOBAL(Trace::Information, (_T("Could not open file: %s"), filename));
            }

            return result;
        }

        uint32_t GetMFRData(mfrSerializedType_t type, string& response)
        {
            uint32_t result = Core::ERROR_GENERAL;

            IARM_Bus_MFRLib_GetSerializedData_Param_t param;
            param.bufLen = 0;
            param.type = type;
            auto status = IARM_Bus_Call(
                IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_GetSerializedData, &param, sizeof(param));
            // FIX(Coverity): Unsafe API Usage - Validate buffer length before use
            // Reason: param.bufLen could overflow buffer size, need bounds checking
            // Impact: No API signature changes. Added validation to prevent buffer overflow.
            if ((status == IARM_RESULT_SUCCESS) && param.bufLen > 0 && param.bufLen <= static_cast<int>(sizeof(param.buffer))) {
                response.assign(param.buffer, param.bufLen);
                result = Core::ERROR_NONE;
            } else {
                if (status == IARM_RESULT_SUCCESS && param.bufLen > static_cast<int>(sizeof(param.buffer))) {
                    TRACE_GLOBAL(Trace::Error, (_T("MFR buffer overflow prevented: bufLen=%d exceeds buffer size"), param.bufLen));
                } else {
                    TRACE_GLOBAL(Trace::Information, (_T("MFR error [%d] for %d"), status, type));
                }
            }

            return result;
        }

        uint32_t GetRFCData(const char* name, string& response)
        {
            uint32_t result = Core::ERROR_GENERAL;

            RFC_ParamData_t param;
            auto status = getRFCParameter(nullptr, name, &param);
            if ((status == WDMP_SUCCESS) && param.value[0]) {
                response = param.value;
                result = Core::ERROR_NONE;
            } else {
                TRACE_GLOBAL(Trace::Information, (_T("RFC error [%d] for %s"), status, name));
            }

            return result;
        }
    }

    SERVICE_REGISTRATION(DeviceInfoImplementation, 1, 0);

    DeviceInfoImplementation::DeviceInfoImplementation()
    {
        Utils::IARM::init();
    }

    Core::hresult DeviceInfoImplementation::SerialNumber(string& serialNumber) const
    {
        return (GetMFRData(mfrSERIALIZED_TYPE_SERIALNUMBER, serialNumber)
                   == Core::ERROR_NONE)
            ? Core::ERROR_NONE
            : GetRFCData(_T("Device.DeviceInfo.SerialNumber"), serialNumber);
    }

    Core::hresult DeviceInfoImplementation::Sku(string& sku) const
    {
        LOGINFO("ENTERING SKU TO GET device.properties");
        return (GetFileRegex(_T("/etc/device.properties"),
                    std::regex("^MODEL_NUM(?:\\s*)=(?:\\s*)(?:\"{0,1})([^\"\\n]+)(?:\"{0,1})(?:\\s*)$"), sku)
                   == Core::ERROR_NONE)
            ? Core::ERROR_NONE
            : ((GetMFRData(mfrSERIALIZED_TYPE_MODELNAME, sku)
                   == Core::ERROR_NONE)
                    ? Core::ERROR_NONE
                    : GetRFCData(_T("Device.DeviceInfo.ModelName"), sku));
    }

    Core::hresult DeviceInfoImplementation::Make(string& make) const
    {

        return ( GetMFRData(mfrSERIALIZED_TYPE_MANUFACTURER, make) == Core::ERROR_NONE)
            ? Core::ERROR_NONE
            : GetFileRegex(_T("/etc/device.properties"),std::regex("^MFG_NAME(?:\\s*)=(?:\\s*)(?:\"{0,1})([^\"\\n]+)(?:\"{0,1})(?:\\s*)$"), make);
    }

    Core::hresult DeviceInfoImplementation::Model(string& model) const
    {
        return
#ifdef ENABLE_DEVICE_MANUFACTURER_INFO
            (GetMFRData(mfrSERIALIZED_TYPE_PROVISIONED_MODELNAME, model) == Core::ERROR_NONE)
            ? Core::ERROR_NONE
            :
#endif
            GetFileRegex(_T("/etc/device.properties"),
                std::regex("^FRIENDLY_ID(?:\\s*)=(?:\\s*)(?:\"{0,1})([^\"\\n]+)(?:\"{0,1})(?:\\s*)$"), model);
    }

    Core::hresult DeviceInfoImplementation::Brand(string& brand) const
    {
        // FIX(Coverity): Logic Error - Ternary Expression - Break into separate statements
        // Reason: Complex nested ternary is difficult to debug and error-prone
        // Impact: No API signature changes. Improved code clarity and maintainability.
        brand = "Unknown";
        
        if (Core::ERROR_NONE == GetFileRegex(_T("/tmp/.manufacturer"), std::regex("^([^\\n]+)$"), brand)) {
            return Core::ERROR_NONE;
        }
        
        if (GetMFRData(mfrSERIALIZED_TYPE_MANUFACTURER, brand) == Core::ERROR_NONE) {
            return Core::ERROR_NONE;
        }
        
        return Core::ERROR_GENERAL;
    }

    Core::hresult DeviceInfoImplementation::DeviceType(string& deviceType) const
    {
        const char* device_type;
        uint32_t result = GetFileRegex(_T("/etc/authService.conf"),
            std::regex("^deviceType(?:\\s*)=(?:\\s*)(?:\"{0,1})([^\"\\n]+)(?:\"{0,1})(?:\\s*)$"), deviceType);

        if (result != Core::ERROR_NONE) {
            // If we didn't find the deviceType in authService.conf, try device.properties
            result = GetFileRegex(_T("/etc/device.properties"),
                std::regex("^DEVICE_TYPE(?:\\s*)=(?:\\s*)(?:\"{0,1})([^\"\\n]+)(?:\"{0,1})(?:\\s*)$"), deviceType);

            if (result == Core::ERROR_NONE) {
                // Perform the conversion logic if we found the deviceType in device.properties
                // as it doesnt comply with plugin spec. See RDKEMW-276
                device_type = deviceType.c_str();
                deviceType = (strcmp("mediaclient", device_type) == 0) ? "IpStb" :
                    (strcmp("hybrid", device_type) == 0) ? "QamIpStb" : "IpTv";
            }
        }
        return result;
    }


    Core::hresult DeviceInfoImplementation::SocName(string& socName)  const
    {
        return (GetFileRegex(_T("/etc/device.properties"),
                std::regex("^SOC(?:\\s*)=(?:\\s*)(?:\"{0,1})([^\"\\n]+)(?:\"{0,1})(?:\\s*)$"), socName));
    }

    Core::hresult DeviceInfoImplementation::DistributorId(string& distributorId) const
    {
        return (GetFileRegex(_T("/opt/www/authService/partnerId3.dat"),
                    std::regex("^([^\\n]+)$"), distributorId)
                   == Core::ERROR_NONE)
            ? Core::ERROR_NONE
            : GetRFCData(_T("Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId"), distributorId);
    }

        Core::hresult DeviceInfoImplementation::ReleaseVersion(string& releaseVersion ) const
    {
        // FIX(Coverity): Code Quality - Return appropriate error code on failure
        // Reason: Function should indicate when default value is used due to errors
        // Impact: No API signature changes. Better error reporting when using fallback value.
        const std::string defaultVersion = "99.99.0.0";
        std::regex pattern(R"((\d+)\.(\d+)[sp])");
        std::smatch match;
        std::string imagename = "";
        uint32_t result = Core::ERROR_NONE;
        
        if(Core::ERROR_NONE == GetFileRegex(_T("/version.txt"), std::regex("^imagename:([^\\n]+)$"), imagename))
        {
            if (std::regex_search(imagename, match, pattern)) {
                std::string major = match[1];
                std::string minor = match[2];
                releaseVersion = major + "." + minor + ".0.0";
            }
            else
            {
                releaseVersion = defaultVersion ;
                LOGERR("Unable to get releaseVersion of the Image:%s.So default releaseVersion is: %s ",imagename.c_str(),releaseVersion.c_str());
                result = Core::ERROR_GENERAL;
            }
        }
        else
        {
                releaseVersion = defaultVersion ;
                LOGERR("Unable to read from /version.txt So default releaseVersion is: %s ",releaseVersion.c_str());
                result = Core::ERROR_GENERAL;
        }

        return result;
    }

    uint32_t DeviceInfoImplementation::ChipSet(string& chipset) const
    {
        auto result = GetFileRegex(_T("/etc/device.properties"),std::regex("^CHIPSET_NAME(?:\\s*)=(?:\\s*)(?:\"{0,1})([^\"\\n]+)(?:\"{0,1})(?:\\s*)$"), chipset);
        return result;
    }
}
}
