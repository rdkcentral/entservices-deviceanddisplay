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

#include "DeviceSettingsDisplayImplementation.h"

#include "UtilsLogging.h"
#include <syscall.h>

using namespace std;

namespace WPEFramework {
namespace Plugin {

    DeviceSettingsDisplayImpl::DeviceSettingsDisplayImpl() : 
        _DisplayNotifications(),
        _DisplayHDMIHotPlugNotifications(),
        _apiLock(),
        _callbackLock(),
        _display(Display::Create(*this))
    {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsDisplayImpl Constructor - Instance Address: %p", this);
        EXIT_LOG;
    }

    DeviceSettingsDisplayImpl::~DeviceSettingsDisplayImpl() {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsDisplayImpl Destructor - Instance Address: %p", this);
        EXIT_LOG;
    }

    template<typename Func, typename... Args>
    void DeviceSettingsDisplayImpl::dispatchDisplayEvent(Func notifyFunc, Args&&... args) {
        LOGINFO(">>");
        _callbackLock.Lock();
        for (auto& notification : _DisplayNotifications) {
            auto start = std::chrono::steady_clock::now();
            (notification->*notifyFunc)(std::forward<Args>(args)...);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("client %p took %" PRId64 "ms to process IDisplay event", notification, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }
        _callbackLock.Unlock();
        LOGINFO("<<");
    }

    template<typename Func, typename... Args>
    void DeviceSettingsDisplayImpl::dispatchDisplayHDMIHotPlugEvent(Func notifyFunc, Args&&... args) {
        LOGINFO(">>");
        _callbackLock.Lock();
        for (auto& notification : _DisplayHDMIHotPlugNotifications) {
            auto start = std::chrono::steady_clock::now();
            (notification->*notifyFunc)(std::forward<Args>(args)...);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("client %p took %" PRId64 "ms to process IDisplayHDMIHotPlug event", notification, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }
        _callbackLock.Unlock();
        LOGINFO("<<");
    }

    template <typename T>
    Core::hresult DeviceSettingsDisplayImpl::Register(std::list<T*>& list, T* notification)
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
    Core::hresult DeviceSettingsDisplayImpl::Unregister(std::list<T*>& list, const T* notification)
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

    Core::hresult DeviceSettingsDisplayImpl::Register(IDisplayNotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Register(_DisplayNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IDisplay %p, errorCode: %u", notification, errorCode);
        } else {
            LOGINFO("IDisplay %p registered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    Core::hresult DeviceSettingsDisplayImpl::Unregister(IDisplayNotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Unregister(_DisplayNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IDisplay %p, errorcode: %u", notification, errorCode);
        } else {
            LOGINFO("IDisplay %p unregistered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    Core::hresult DeviceSettingsDisplayImpl::Register(IDisplayHDMIHotPlugNotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Register(_DisplayHDMIHotPlugNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IDisplayHDMIHotPlug %p, errorCode: %u", notification, errorCode);
        } else {
            LOGINFO("IDisplayHDMIHotPlug %p registered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    Core::hresult DeviceSettingsDisplayImpl::Unregister(IDisplayHDMIHotPlugNotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Unregister(_DisplayHDMIHotPlugNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IDisplayHDMIHotPlug %p, errorcode: %u", notification, errorCode);
        } else {
            LOGINFO("IDisplayHDMIHotPlug %p unregistered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    void DeviceSettingsDisplayImpl::OnDisplayRxSense(const DisplayEvent displayEvent)
    {
        ENTRY_LOG;
        LOGINFO("DS HAL OnDisplayRxSense event: displayEvent=%d", static_cast<int>(displayEvent));
        dispatchDisplayEvent(&IDisplayNotification::OnDisplayRxSense, displayEvent);
        EXIT_LOG;
    }

    void DeviceSettingsDisplayImpl::OnDisplayHDCPStatus()
    {
        ENTRY_LOG;
        LOGINFO("DS HAL OnDisplayHDCPStatus event");
        dispatchDisplayEvent(&IDisplayNotification::OnDisplayHDCPStatus);
        EXIT_LOG;
    }

    void DeviceSettingsDisplayImpl::OnDisplayHDMIHotPlug(const DisplayEvent displayEvent)
    {
        ENTRY_LOG;
        LOGINFO("DS HAL OnDisplayHDMIHotPlug event: displayEvent=%d", static_cast<int>(displayEvent));
        dispatchDisplayHDMIHotPlugEvent(&IDisplayHDMIHotPlugNotification::OnDisplayHDMIHotPlug, displayEvent);
        EXIT_LOG;
    }

    // Display interface method implementations called by DeviceSettingsImp 
    uint32_t DeviceSettingsDisplayImpl::GetDisplayEdid(const int32_t handle, DisplayEDID &edId, IDSVideoPortResolutionIterator*& supportedResolutionList)
    {
        ENTRY_LOG;
        uint32_t result = Core::ERROR_GENERAL;
        result = _display.GetDisplayEdid(handle, edId, supportedResolutionList);
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetDisplayEdid succeeded: handle=%d", handle);
        } else {
            LOGERR("GetDisplayEdid failed: handle=%d, error=%u", handle, result);
        }
        EXIT_LOG;
        return result;
    }

    uint32_t DeviceSettingsDisplayImpl::GetDisplayEdidBytes(const int32_t handle, uint8_t edIdBytes[], const uint16_t edidLength)
    {
        ENTRY_LOG;
        uint32_t result = Core::ERROR_GENERAL;
        result = _display.GetDisplayEdidBytes(handle, edIdBytes, edidLength);
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetDisplayEdidBytes succeeded: handle=%d, edidLength=%d", handle, edidLength);
        } else {
            LOGERR("GetDisplayEdidBytes failed: handle=%d, edidLength=%d, error=%u", handle, edidLength, result);
        }
        EXIT_LOG;
        return result;
    }

    uint32_t DeviceSettingsDisplayImpl::GetDisplay(const DisplayPortType portType, const int32_t index, int32_t &handle)
    {
        ENTRY_LOG;
        uint32_t result = Core::ERROR_GENERAL;
        result = _display.GetDisplay(portType, index, handle);
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetDisplay succeeded: portType=%d, index=%d, handle=%d", static_cast<int>(portType), index, handle);
        } else {
            LOGERR("GetDisplay failed: portType=%d, index=%d, error=%u", static_cast<int>(portType), index, result);
        }
        EXIT_LOG;
        return result;
    }

    uint32_t DeviceSettingsDisplayImpl::GetDisplayAspectRatio(const int32_t handle, Exchange::IDeviceSettingsDisplay::DisplayVideoAspectRatio &aspectRatio)
    {
        ENTRY_LOG;
        uint32_t result = Core::ERROR_GENERAL;
        result = _display.GetDisplayAspectRatio(handle, aspectRatio);
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetDisplayAspectRatio succeeded: handle=%d, aspectRatio=%d", handle, static_cast<int>(aspectRatio));
        } else {
            LOGERR("GetDisplayAspectRatio failed: handle=%d, error=%u", handle, result);
        }
        EXIT_LOG;
        return result;
    }

    uint32_t DeviceSettingsDisplayImpl::SetAllmEnabled(const int32_t handle, const bool enabled)
    {
        ENTRY_LOG;
        uint32_t result = Core::ERROR_GENERAL;
        result = _display.SetAllmEnabled(handle, enabled);
        if (result == Core::ERROR_NONE) {
            LOGINFO("SetAllmEnabled succeeded: handle=%d, enabled=%s", handle, enabled ? "true" : "false");
        } else {
            LOGERR("SetAllmEnabled failed: handle=%d, enabled=%s, error=%u", handle, enabled ? "true" : "false", result);
        }
        EXIT_LOG;
        return result;
    }

    uint32_t DeviceSettingsDisplayImpl::SetAVIContentType(const int32_t handle, const DisplayAVIContentType contentType)
    {
        ENTRY_LOG;
        uint32_t result = Core::ERROR_GENERAL;
        result = _display.SetAVIContentType(handle, contentType);
        if (result == Core::ERROR_NONE) {
            LOGINFO("SetAVIContentType succeeded: handle=%d, contentType=%d", handle, static_cast<int>(contentType));
        } else {
            LOGERR("SetAVIContentType failed: handle=%d, contentType=%d, error=%u", handle, static_cast<int>(contentType), result);
        }
        EXIT_LOG;
        return result;
    }

    uint32_t DeviceSettingsDisplayImpl::SetAVIScanInformation(const int32_t handle, const DisplayAVIScanInformation scanInfo)
    {
        ENTRY_LOG;
        uint32_t result = Core::ERROR_GENERAL;
        result = _display.SetAVIScanInformation(handle, scanInfo);
        if (result == Core::ERROR_NONE) {
            LOGINFO("SetAVIScanInformation succeeded: handle=%d, scanInfo=%d", handle, static_cast<int>(scanInfo));
        } else {
            LOGERR("SetAVIScanInformation failed: handle=%d, scanInfo=%d, error=%u", handle, static_cast<int>(scanInfo), result);
        }
        EXIT_LOG;
        return result;
    }

} // namespace Plugin
} // namespace WPEFramework