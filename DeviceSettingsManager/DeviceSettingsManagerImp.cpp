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
        ENTRY_LOG;
        DeviceSettingsManagerImp::_instance = this;
        DeviceManager_Init();
        LOGINFO("DeviceSettingsManagerImp Constructor");
        EXIT_LOG;
    }

    DeviceSettingsManagerImp::~DeviceSettingsManagerImp() {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsManagerImp Destructor");
        EXIT_LOG;
    }

    template <typename T>
    Core::hresult DeviceSettingsManagerImp::Register(std::list<T*>& list, T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;
        ENTRY_LOG;
        ASSERT(nullptr != notification);
        _callbackLock.Lock();

        // Make sure we can't register the same notification callback multiple times
        if (std::find(list.begin(), list.end(), notification) == list.end()) {
            list.push_back(notification);
            notification->AddRef();
            status = Core::ERROR_NONE;
        }

        _callbackLock.Unlock();
        EXIT_LOG;
        return status;
    }

    template <typename T>
    Core::hresult DeviceSettingsManagerImp::Unregister(std::list<T*>& list, const T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;
        ENTRY_LOG;
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
        EXIT_LOG;
        return status;
    }

    void DeviceSettingsManagerImp::DeviceManager_Init()
    {
        ENTRY_LOG;
        dsMgr_init();
        EXIT_LOG;
    }

    Core::hresult DeviceSettingsManagerImp::Register(Exchange::IDeviceSettingsManagerFPD::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Register(_FPDNotifications, notification);
        LOGINFO("IDeviceSettingsManagerAudio %p, errorCode: %u", notification, errorCode);
        EXIT_LOG;
        return errorCode;
    }
    Core::hresult DeviceSettingsManagerImp::Unregister(Exchange::IDeviceSettingsManagerFPD::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Unregister(_FPDNotifications, notification);
        LOGINFO("IModeChanged %p, errorcode: %u", notification, errorCode);
        EXIT_LOG;
        return errorCode;
    }

    //Depricated
    Core::hresult DeviceSettingsManagerImp::SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) {
        ENTRY_LOG;
        LOGINFO("SetFPDTime: timeFormat=%d, minutes=%u, seconds=%u", timeFormat, minutes, seconds);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) {
        ENTRY_LOG;
        LOGINFO("SetFPDScroll: scrollHoldDuration=%u, horizontal=%u, vertical=%u", scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) {
        ENTRY_LOG;
        LOGINFO("SetFPDTextBrightness: textDisplay=%d, brightNess=%u", textDisplay, brightNess);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) {
        ENTRY_LOG;
        brightNess = 50; // Example value
        LOGINFO("GetFPDTextBrightness: textDisplay=%d", textDisplay);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::EnableFPDClockDisplay(const bool enable) {
        ENTRY_LOG;
        LOGINFO("EnableFPDClockDisplay: enable=%s", enable ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) {
        ENTRY_LOG;
        fpdTimeFormat = FPDTimeFormat::DS_FPD_TIMEFORMAT_24_HOUR; // Example value
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) {
        ENTRY_LOG;
        LOGINFO("SetFPDTimeFormat: fpdTimeFormat=%d", fpdTimeFormat);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }
    //Depricated

    Core::hresult DeviceSettingsManagerImp::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
        ENTRY_LOG;
        LOGINFO("SetFPDBlink: indicator=%d, blinkDuration=%u, blinkIterations=%u", indicator, blinkDuration, blinkIterations);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) {
        ENTRY_LOG;
        LOGINFO("SetFPDBrightness: indicator=%d, brightNess=%u, persist=%s", indicator, brightNess, persist ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) {
        ENTRY_LOG;
        LOGINFO("GetFPDBrightness: indicator=%d", indicator);
        //brightNess = 50; // Example value
        _fpd.GetFPDBrightness(indicator, brightNess);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDState(const FPDIndicator indicator, const FPDState state) {
        ENTRY_LOG;
        LOGINFO("SetFPDState: indicator=%d, state=%d", indicator, state);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDState(const FPDIndicator indicator, FPDState &state) {
        ENTRY_LOG;
        LOGINFO("GetFPDState: indicator=%d", indicator);
        state = FPDState::DS_FPD_STATE_ON; // Example value
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
        ENTRY_LOG;
        LOGINFO("GetFPDColor: indicator=%d", indicator);
        color = 0xFFFFFF; // Example value
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
        ENTRY_LOG;
        LOGINFO("SetFPDColor: indicator=%d, color=0x%X", indicator, color);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDMode(const FPDMode fpdMode) {
        ENTRY_LOG;
        LOGINFO("SetFPDMode: fpdMode=%d", fpdMode);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }
} // namespace Plugin
} // namespace WPEFramework