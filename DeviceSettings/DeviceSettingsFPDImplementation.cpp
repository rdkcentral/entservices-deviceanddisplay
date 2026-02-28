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

#include "DeviceSettingsFPDImplementation.h"

#include "UtilsLogging.h"
#include <syscall.h>

using namespace std;

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceSettingsFPDImpl, 1, 0);

    DeviceSettingsFPDImpl* DeviceSettingsFPDImpl::_instance = nullptr;

    DeviceSettingsFPDImpl::DeviceSettingsFPDImpl()
        : _fpd(FPD::Create(*this))
    {
        ENTRY_LOG;
        DeviceSettingsFPDImpl::_instance = this;
        LOGINFO("DeviceSettingsFPDImpl Constructor - Instance Address: %p", this);
        EXIT_LOG;
    }

    DeviceSettingsFPDImpl::~DeviceSettingsFPDImpl() {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsFPDImpl Destructor - Instance Address: %p", this);
        EXIT_LOG;
    }


    template<typename Func, typename... Args>
    void DeviceSettingsFPDImpl::dispatchFPDEvent(Func notifyFunc, Args&&... args) {
        LOGINFO(">>");
        _callbackLock.Lock();
        for (auto& notification : _FPDNotifications) {
            auto start = std::chrono::steady_clock::now();
            (notification->*notifyFunc)(std::forward<Args>(args)...);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("client %p took %" PRId64 "ms to process IFPD event", notification, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }
        _callbackLock.Unlock();
        LOGINFO("<<");
    }

    template <typename T>
    Core::hresult DeviceSettingsFPDImpl::Register(std::list<T*>& list, T* notification)
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
        } else {
            LOGWARN("Notification %p already registered - skipping", notification);
        }
        _callbackLock.Unlock();

        EXIT_LOG;
        return status;
    }

    template <typename T>
    Core::hresult DeviceSettingsFPDImpl::Unregister(std::list<T*>& list, const T* notification)
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

    Core::hresult DeviceSettingsFPDImpl::Register(DeviceSettingsFPD::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Register(_FPDNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IFPD %p, errorCode: %u", notification, errorCode);
        } else {
            LOGINFO("IFPD %p registered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    Core::hresult DeviceSettingsFPDImpl::Unregister(DeviceSettingsFPD::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Unregister(_FPDNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IFPD %p, errorcode: %u", notification, errorCode);
        } else {
            LOGINFO("IFPD %p unregistered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    // FPD notification implementation
    void DeviceSettingsFPDImpl::OnFPDTimeFormatChanged(const FPDTimeFormat timeFormat)
    {
        ENTRY_LOG;
        LOGINFO("OnFPDTimeFormatChanged event Received: timeFormat=%d", timeFormat);
        dispatchFPDEvent(&DeviceSettingsFPD::INotification::OnFPDTimeFormatChanged, timeFormat);
        EXIT_LOG;
    }

    //Depricated
    Core::hresult DeviceSettingsFPDImpl::SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) {
        ENTRY_LOG;
        LOGINFO("SetFPDTime: timeFormat=%d, minutes=%u, seconds=%u", timeFormat, minutes, seconds);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) {
        ENTRY_LOG;
        LOGINFO("SetFPDScroll: scrollHoldDuration=%u, horizontal=%u, vertical=%u", scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) {
        ENTRY_LOG;
        LOGINFO("SetFPDTextBrightness: textDisplay=%d, brightNess=%u", textDisplay, brightNess);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) {
        ENTRY_LOG;
        brightNess = 50; // Example value
        LOGINFO("GetFPDTextBrightness: textDisplay=%d", textDisplay);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::EnableFPDClockDisplay(const bool enable) {
        ENTRY_LOG;
        LOGINFO("EnableFPDClockDisplay: enable=%s", enable ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) {
        ENTRY_LOG;
        fpdTimeFormat = FPDTimeFormat::DS_FPD_TIMEFORMAT_24_HOUR; // Example value
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) {
        ENTRY_LOG;
        LOGINFO("SetFPDTimeFormat: fpdTimeFormat=%d", fpdTimeFormat);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
        ENTRY_LOG;
        LOGINFO("SetFPDBlink: indicator=%d, blinkDuration=%u, blinkIterations=%u", indicator, blinkDuration, blinkIterations);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::SetFPDMode(const FPDMode fpdMode) {
        ENTRY_LOG;
        LOGINFO("SetFPDMode: fpdMode=%d", fpdMode);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }
    //Depricated

    Core::hresult DeviceSettingsFPDImpl::SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) {
        ENTRY_LOG;
        LOGINFO("SetFPDBrightness: indicator=%d, brightNess=%u, persist=%s", indicator, brightNess, persist ? "true" : "false");

        _apiLock.Lock();
        _fpd.SetFPDBrightness(indicator, brightNess, persist);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) {
        ENTRY_LOG;

        _apiLock.Lock();
        _fpd.GetFPDBrightness(indicator, brightNess);
        _apiLock.Unlock();

        LOGINFO("GetFPDBrightness: indicator=%d, brightNess=%d", indicator, brightNess);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::SetFPDState(const FPDIndicator indicator, const FPDState state) {
        ENTRY_LOG;
        LOGINFO("SetFPDState: indicator=%d, state=%d", indicator, state);

        _apiLock.Lock();
        _fpd.SetFPDState(indicator, state);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::GetFPDState(const FPDIndicator indicator, FPDState &state) {
        ENTRY_LOG;

        _apiLock.Lock();
        _fpd.GetFPDState(indicator, state);
        _apiLock.Unlock();

        LOGINFO("GetFPDState: indicator=%d, state=%d", indicator, state);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
        ENTRY_LOG;

        _apiLock.Lock();
        _fpd.GetFPDColor(indicator, color);
        _apiLock.Unlock();

        LOGINFO("GetFPDColor: indicator=%d, color=0x%X", indicator, color);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsFPDImpl::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
        ENTRY_LOG;
        LOGINFO("SetFPDColor: indicator=%d, color=0x%X", indicator, color);

        _apiLock.Lock();
        _fpd.SetFPDColor(indicator, color);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
