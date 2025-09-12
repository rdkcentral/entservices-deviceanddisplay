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

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {

namespace Plugin
{
    SERVICE_REGISTRATION(DeviceSettingsManager, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    namespace {
        static Metadata<DeviceSettingsManager> metadata(
            // Version
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
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
        , mDeviceSettingsManagerAudio(nullptr)
        , mNotificationSink(this)

    {
    }

    const string DeviceSettingsManager::Initialize(PluginHost::IShell * service)
    {
        ASSERT(service != nullptr);
        ASSERT(mService == nullptr);
        ASSERT(mConnectionId == 0);
        ASSERT(mDeviceSettingsManagerAudio == nullptr);
        mService = service;
        mService->AddRef();

        LOGINFO();
        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        mService->Register(&mNotificationSink);

        /*mDeviceSettingsManagerAudio = service->Root<Exchange::IDeviceSettingsManager>(mConnectionId, RPC::CommunicationTimeOut, _T("DeviceSettingsManagerImp"));
        if (mDeviceSettingsManagerAudio != nullptr) {
            mDeviceSettingsManagerAudio->Initialize(service);
            mDeviceSettingsManagerAudio->Register(&mNotificationSink);
            Exchange::IDeviceSettingsManagerAudio::Register(*this, mDeviceSettingsManagerAudio);
        }*/
        // On success return empty, to indicate there is no error text.
        return (string());
    }

    void DeviceSettingsManager::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        LOGINFO();
        if (mService != nullptr) {
            ASSERT(mService == service);
            mService->Unregister(&mNotificationSink);

            /*if (mDeviceSettingsManagerAudio != nullptr) {
                mDeviceSettingsManagerAudio->Unregister(&mNotificationSink);
                Exchange::IDeviceSettingsManagerAudio::Unregister(*this);
            }*/

            mService->Release();
            mService = nullptr;
            mConnectionId = 0;
            SYSLOG(Logging::Shutdown, (string(_T("DeviceSettingsManager de-initialised"))));
        }
    }

    string DeviceSettingsManager::Information() const
    {
        // No additional info to report.
        return (string());
    }


    void DeviceSettingsManager::Deactivated(RPC::IRemoteConnection* connection)
    {
        LOGINFO();
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (mConnectionId == connection->Id()) {
            ASSERT(mService != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(mService, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin
} // namespace WPEFramework
