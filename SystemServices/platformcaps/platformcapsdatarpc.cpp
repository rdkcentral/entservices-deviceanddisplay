/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2020 RDK Management
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

#include "platformcapsdata.h"

#include <regex>

namespace {
  string stringFromHex(const string &hex) {
    string result;

    for (int i = 0, len = hex.length(); i < len; i += 2) {
      auto byte = hex.substr(i, 2);
      char chr = (char) (int) strtol(byte.c_str(), nullptr, 16);
      result.push_back(chr);
    }

    return result;
  }
}

namespace WPEFramework {
namespace Plugin {
  uint32_t GetFileRegex(const char* filename, const std::regex& regex, string& response){
    uint32_t result = Core::ERROR_GENERAL;
    std::ifstream file(filename);
    if (file) {
      string line;
      while (std::getline(file, line)) {
        std::smatch sm;
        if (std::regex_match(line, sm, regex)) {
          ASSERT(sm.size() == 2);
          response = sm[1];
          result = Core::ERROR_NONE;
          break;
        }
      }
    }
    return result;
  }

string PlatformCapsData::GetModel() {
  return jsonRpc.invoke(_T("org.rdk.System"),
                        _T("getDeviceInfo"), 5000)
      .Get(_T("model_number")).String();
}

string PlatformCapsData::GetDeviceType() {
  const char* device_type;
  string deviceType;
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
        (strcmp("hybrid", device_type) == 0) ? "QamIpStb" : "TV";
    }
  }
  return deviceType;
}

string PlatformCapsData::GetHDRCapability() {
  JsonArray hdrCaps = jsonRpc.invoke(_T("org.rdk.DisplaySettings"),
                                     _T("getSettopHDRSupport"), 3000)
      .Get(_T("standards")).Array();

  string result;

  JsonArray::Iterator index(hdrCaps.Elements());
  while (index.Next() == true) {
    if (!result.empty())
      result.append(",");
    result.append(index.Current().String());
  }

  return result;
}

string PlatformCapsData::GetAccountId() {
  return jsonRpc.invoke(_T("org.rdk.AuthService"),
                        _T("getAlternateIds"), 3000)
      .Get(_T("alternateIds")).Object().Get(_T("_xbo_account_id")).String();
}

string PlatformCapsData::GetX1DeviceId() {
  return jsonRpc.invoke(_T("org.rdk.AuthService"),
                        _T("getXDeviceId"), 3000)
      .Get(_T("xDeviceId")).String();
}

bool PlatformCapsData::XCALSessionTokenAvailable() {
  string tkn = jsonRpc.invoke(_T("org.rdk.AuthService"),
                              _T("getSessionToken"), 10000)
      .Get(_T("token")).String();
  return (!tkn.empty());
}

string PlatformCapsData::GetExperience() {
  return jsonRpc.invoke(_T("org.rdk.AuthService"),
                        _T("getExperience"), 3000)
      .Get(_T("experience")).String();
}

string PlatformCapsData::GetDdeviceMACAddress() {
  return jsonRpc.invoke(_T("org.rdk.System"),
                        _T("getDeviceInfo"), 5000)
      .Get(_T("estb_mac")).String();
}

string PlatformCapsData::GetPublicIP() {
  return jsonRpc.invoke(_T("org.rdk.Network"),
                        _T("getPublicIP"), 5000)
      .Get(_T("public_ip")).String();
}

JsonObject PlatformCapsData::JsonRpc::invoke(const string &callsign,
    const string &method, const uint32_t waitTime) {
  JsonObject params, result;

  auto err = getClient(callsign)->Invoke<JsonObject, JsonObject>(
      waitTime, method, params, result);

  if (err != Core::ERROR_NONE) {
    TRACE(Trace::Error, (_T("%s JsonRpc %" PRId32 " (%s.%s)\n"),
        __FILE__, err, callsign.c_str(), method.c_str()));
  }

  return result;
}

bool PlatformCapsData::JsonRpc::activate(const string &callsign,
    const uint32_t waitTime) {
  JsonObject params, result;
  params["callsign"] = callsign;

  auto err = getClient(_T("Controller"))->Invoke<JsonObject, JsonObject>(
      waitTime, _T("activate"), params, result);

  if (err != Core::ERROR_NONE) {
    TRACE(Trace::Error, (_T("%s JsonRpc %" PRId32 " (%s)\n"),
        __FILE__, err, callsign.c_str()));
  }

  return (err == Core::ERROR_NONE);
}

PlatformCapsData::JsonRpc::ClientProxy PlatformCapsData::JsonRpc::getClient(
    const string &callsign) {
  auto retval = clients.emplace(std::piecewise_construct,
                                std::make_tuple(callsign),
                                std::make_tuple());

  if (retval.second == true) {
    if (!callsign.empty() && callsign != _T("Controller")) {
      activate(callsign, 3000);
    }

    retval.first->second = ClientProxy::Create(_service, callsign);
  }

  return retval.first->second;
}

} // namespace Plugin
} // namespace WPEFramework
