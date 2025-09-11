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

#include "DeviceSettingsManagerImp.h"
#include <interfaces/IConfiguration.h>

#include "UtilsLogging.h"

using namespace std;

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceSettingsManagerImp, 1, 0);
    DeviceSettingsManagerImp* DeviceSettingsManagerImp::_instance = nullptr;

    DeviceSettingsManagerImp::DeviceSettingsManagerImp()
        : _fpd()
    {
        DeviceSettingsManagerImp::_instance = this;
        DeviceManager_Init();
        LOGINFO("DeviceSettingsManagerImp Constructor");
    }

    DeviceSettingsManagerImp::~DeviceSettingsManagerImp() {
        LOGINFO("DeviceSettingsManagerImp Destructor");
    }

    template <typename T>
    Core::hresult DeviceSettingsManagerImp::Register(std::list<T*>& list, T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);
        _callbackLock.Lock();

        // Make sure we can't register the same notification callback multiple times
        if (std::find(list.begin(), list.end(), notification) == list.end()) {
            list.push_back(notification);
            notification->AddRef();
            status = Core::ERROR_NONE;
        }

        _callbackLock.Unlock();
        return status;
    }

    template <typename T>
    Core::hresult DeviceSettingsManagerImp::Unregister(std::list<T*>& list, const T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);
        _callbackLock.Lock();

        // Make sure we can't unregister the same notification callback multiple times
        auto itr = std::find(list.begin(), list.end(), notification);
        if (itr != list.end()) {
            (*itr)->Release();
            list.erase(itr);
            status = Core::ERROR_NONE;
        }

        _callbackLock.Unlock();
        return status;
    }

    void DeviceManager_Init()
    {
        LOGINFO("DeviceManager_Init");
        dsMgr_init();
    }

    Core::hresult DeviceSettingsManagerImp::Register(Exchange::IDeviceSettingsManagerFPD::INotification* notification)
    {
        Core::hresult errorCode = Register(_FPDNotifications, notification);
        LOGINFO("IDeviceSettingsManagerAudio %p, errorCode: %u", notification, errorCode);
        return errorCode;
    }
    Core::hresult DeviceSettingsManagerImp::Unregister(Exchange::IDeviceSettingsManagerFPD::INotification* notification)
    {
        Core::hresult errorCode = Unregister(_FPDNotifications, notification);
        LOGINFO("IModeChanged %p, errorcode: %u", notification, errorCode);
        return errorCode;
    }

    //Depricated
    Core::hresult DeviceSettingsManagerImp::SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) {
        LOGINFO("SetFPDTime: timeFormat=%d, minutes=%u, seconds=%u", timeFormat, minutes, seconds);
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) {
        LOGINFO("SetFPDScroll: scrollHoldDuration=%u, horizontal=%u, vertical=%u", scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations);
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) {
        LOGINFO("SetFPDTextBrightness: textDisplay=%d, brightNess=%u", textDisplay, brightNess);
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) {
        brightNess = 50; // Example value
        LOGINFO("GetFPDTextBrightness: textDisplay=%d", textDisplay);
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::EnableFPDClockDisplay(const bool enable) {
        LOGINFO("EnableFPDClockDisplay: enable=%s", enable ? "true" : "false");
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) {
        LOGINFO("GetFPDTimeFormat");
        fpdTimeFormat = FPDTimeFormat::DS_FPD_TIMEFORMAT_24_HOUR; // Example value
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) {
        LOGINFO("SetFPDTimeFormat: fpdTimeFormat=%d", fpdTimeFormat);
        return Core::ERROR_NONE;
    }
    //Depricated

    Core::hresult DeviceSettingsManagerImp::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
        LOGINFO("SetFPDBlink: indicator=%d, blinkDuration=%u, blinkIterations=%u", indicator, blinkDuration, blinkIterations);
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) {
        LOGINFO("SetFPDBrightness: indicator=%d, brightNess=%u, persist=%s", indicator, brightNess, persist ? "true" : "false");
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) {
        LOGINFO("GetFPDBrightness: indicator=%d", indicator);
        //brightNess = 50; // Example value
        _fpd.GetFPDBrightness(indicator, brightNess);
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDState(const FPDIndicator indicator, const FPDState state) {
        LOGINFO("SetFPDState: indicator=%d, state=%d", indicator, state);
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDState(const FPDIndicator indicator, FPDState &state) {
        LOGINFO("GetFPDState: indicator=%d", indicator);
        state = FPDState::DS_FPD_STATE_ON; // Example value
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
        LOGINFO("GetFPDColor: indicator=%d", indicator);
        color = 0xFFFFFF; // Example value
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
        LOGINFO("SetFPDColor: indicator=%d, color=0x%X", indicator, color);
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDMode(const FPDMode fpdMode) {
        LOGINFO("SetFPDMode: fpdMode=%d", fpdMode);
        return Core::ERROR_NONE;
    }
} // namespace Plugin
} // namespace WPEFramework