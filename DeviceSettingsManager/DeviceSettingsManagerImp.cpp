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

#include "UtilsLogging.h"
#include <syscall.h>
#include <type_traits>

using namespace std;

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceSettingsManagerImp, 1, 0);

    DeviceSettingsManagerImp* DeviceSettingsManagerImp::_instance = nullptr;

    DeviceSettingsManagerImp::DeviceSettingsManagerImp()
        : _fpd(FPD::Create(*this))
        , _hdmiIn(HdmiIn::Create(*this))
        , _audio(Audio::Create(*this))
    {
        ENTRY_LOG;
        DeviceSettingsManagerImp::_instance = this;
        LOGINFO("DeviceSettingsManagerImp Constructor - Instance Address: %p", this);

        /*using Impl = WPEFramework::Core::ServiceMetadata<WPEFramework::Plugin::DeviceSettingsManagerImp>
                         ::ServiceImplementation<WPEFramework::Plugin::DeviceSettingsManagerImp>;
        static_assert(!std::is_abstract<Impl>::value, "ServiceImplementation is abstract!");*/

        EXIT_LOG;
    }

    DeviceSettingsManagerImp::~DeviceSettingsManagerImp() {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsManagerImp Destructor - Instance Address: %p", this);
        EXIT_LOG;
    }

    // ================================
    // Common Template Methods for Event Dispatching
    // ================================

    template<typename Func, typename... Args>
    void DeviceSettingsManagerImp::dispatchHDMIInEvent(Func notifyFunc, Args&&... args) {
        LOGINFO(">>");
        _callbackLock.Lock();
        for (auto& notification : _HDMIInNotifications) {
            auto start = std::chrono::steady_clock::now();
            (notification->*notifyFunc)(std::forward<Args>(args)...);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("client %p took %" PRId64 "ms to process IHDMIIn event", notification, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }
        _callbackLock.Unlock();
        LOGINFO("<<");
    }

    template<typename Func, typename... Args>
    void DeviceSettingsManagerImp::dispatchFPDEvent(Func notifyFunc, Args&&... args) {
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

    template<typename Func, typename... Args>
    void DeviceSettingsManagerImp::dispatchAudioEvent(Func notifyFunc, Args&&... args) {
        LOGINFO(">>");
        _callbackLock.Lock();
        for (auto& notification : _AudioNotifications) {
            auto start = std::chrono::steady_clock::now();
            (notification->*notifyFunc)(std::forward<Args>(args)...);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("client %p took %" PRId64 "ms to process IAudio event", notification, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }
        _callbackLock.Unlock();
        LOGINFO("<<");
    }

    // ================================
    // Common Template Methods for Registration
    // ================================

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
        } else {
            LOGWARN("Notification %p already registered - skipping", notification);
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

    // ================================
    // Instance Management
    // ================================

    DeviceSettingsManagerImp* DeviceSettingsManagerImp::instance(DeviceSettingsManagerImp* DeviceSettingsManagerImpl) 
    {
        if (DeviceSettingsManagerImpl != nullptr) {
            _instance = DeviceSettingsManagerImpl;
        }
        return _instance;
    }

} // namespace Plugin
} // namespace WPEFramework

//
// Component-specific implementations are now in:
// - DeviceSettingsManagerFPD.cpp
// - DeviceSettingsManagerHDMIIn.cpp  
// - DeviceSettingsManagerAudio.cpp
