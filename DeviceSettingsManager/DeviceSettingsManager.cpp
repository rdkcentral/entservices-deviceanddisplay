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

#include "DeviceSettingsManager.h"
#include <interfaces/IConfiguration.h>

namespace WPEFramework {

namespace Plugin
{
    SERVICE_REGISTRATION(DeviceSettingsManager, API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);

    namespace {
        static Metadata<DeviceSettingsManager> metadata(
            // Version
            API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    DeviceSettingsManager::DeviceSettingsManager()
        : mConnectionId(0)
        , mService(nullptr)
#ifndef USE_LEGACY_INTERFACE
        , _mDeviceSettingsManager(nullptr)
#endif
        , mNotificationSink(this)

    {
    }

    DeviceSettingsManager::~DeviceSettingsManager()
    {
    }
    const string DeviceSettingsManager::Initialize(PluginHost::IShell * service)
    {
        string message = "";
        ENTRY_LOG;
        ASSERT(service != nullptr);
        ASSERT(mService == nullptr);
        ASSERT(mConnectionId == 0);
#ifdef USE_LEGACY_INTERFACE
        ASSERT(_mDeviceSettingsManagerFPD == nullptr);
#else
        ASSERT(_mDeviceSettingsManager == nullptr);
#endif
        mService = service;
        mService->AddRef();

        mService->Register(mNotificationSink.baseInterface<RPC::IRemoteConnection::INotification>());
        mService->Register(mNotificationSink.baseInterface<PluginHost::IShell::ICOMLink::INotification>());

#ifdef USE_LEGACY_INTERFACE
        _mDeviceSettingsManagerFPD = service->Root<Exchange::IDeviceSettingsManagerFPD>(mConnectionId, RPC::CommunicationTimeOut, _T("DeviceSettingsManagerImp"));

        if (_mDeviceSettingsManagerFPD == nullptr) {
            SYSLOG(Logging::Startup, (_T("DeviceSettingsManager::Initialize: Failed to initialise DeviceSettingsManager plugin")));
            message = _T("DeviceSettingsManager plugin could not be initialised");
            LOGERR("Failed to get IDeviceSettingsManager interface");
        } else {
            LOGINFO("DeviceSettingsManagerImp initialized successfully");
        }
#else
        _mDeviceSettingsManager = service->Root<Exchange::IDeviceSettingsManager>(mConnectionId, RPC::CommunicationTimeOut, _T("DeviceSettingsManagerImp"));

        if (_mDeviceSettingsManager == nullptr) {
            SYSLOG(Logging::Startup, (_T("DeviceSettingsManager::Initialize: Failed to initialise DeviceSettingsManager plugin")));
            message = _T("DeviceSettingsManager plugin could not be initialised");
            LOGERR("Failed to get IDeviceSettingsManager interface");
        } else {
            LOGINFO("DeviceSettingsManagerImp initialized successfully");
        }
#endif
        if (0 != message.length()) {
            Deinitialize(service);
        }
        EXIT_LOG;

        // On success return empty, to indicate there is no error text.
        return (message);
    }

    void DeviceSettingsManager::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ENTRY_LOG;
        if (mService != nullptr) {
            ASSERT(mService == service);
            mService->Unregister(mNotificationSink.baseInterface<RPC::IRemoteConnection::INotification>());
            mService->Unregister(mNotificationSink.baseInterface<PluginHost::IShell::ICOMLink::INotification>());
#ifdef USE_LEGACY_INTERFACE
            _mDeviceSettingsManagerFPD = nullptr;
#else
            _mDeviceSettingsManager = nullptr;
#endif
            mService->Release();
            mService = nullptr;
            mConnectionId = 0;
            SYSLOG(Logging::Shutdown, (string(_T("DeviceSettingsManager de-initialised"))));
        }
        EXIT_LOG;
    }

    string DeviceSettingsManager::Information() const
    {
        ENTRY_LOG;
        // No additional info to report.
        EXIT_LOG;

        return (string());
    }

    void DeviceSettingsManager::Deactivated(RPC::IRemoteConnection* connection)
    {
        ENTRY_LOG;
        // This can potentially be called on a socket thread, so the deactivation (which in turn kills this object) must be done
        // on a separate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (mConnectionId == connection->Id()) {
            ASSERT(mService != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(mService, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
        EXIT_LOG;
    }

    void DeviceSettingsManager::CallbackRevoked(const Core::IUnknown* remote, const uint32_t interfaceId)
    {
        // Add your handling code here, or leave empty if not needed
        LOGINFO("CallbackRevoked called for interfaceId %u", interfaceId);
    }

} // namespace Plugin
} // namespace WPEFramework
