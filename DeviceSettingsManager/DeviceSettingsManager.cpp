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
        , _mDeviceSettingsManagerFPD(nullptr)
        , _mDeviceSettingsManagerHDMIIn(nullptr)
        , mNotificationSink(this)

    {
    }

    DeviceSettingsManager::~DeviceSettingsManager()
    {
        ENTRY_LOG;
        EXIT_LOG;
    }
    const string DeviceSettingsManager::Initialize(PluginHost::IShell * service)
    {
        ENTRY_LOG;
        ASSERT(service != nullptr);
        ASSERT(mService == nullptr);
        ASSERT(mConnectionId == 0);
        ASSERT(_mDeviceSettingsManagerFPD == nullptr);
        ASSERT(_mDeviceSettingsManagerHDMIIn == nullptr);
        mService = service;
        mService->AddRef();

        LOGINFO("Trace - 1");
        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        //mService->Register(&mNotificationSink);
        mService->Register(mNotificationSink.baseInterface<RPC::IRemoteConnection::INotification>());
        mService->Register(mNotificationSink.baseInterface<PluginHost::IShell::ICOMLink::INotification>());

        LOGINFO("Trace - 2");
        _mDeviceSettingsManager = service->Root<Exchange::IDeviceSettingsManager>(mConnectionId, RPC::CommunicationTimeOut, _T("DeviceSettingsManagerImp"));

        /*if (_mDeviceSettingsManagerFPD != nullptr) {
            LOGINFO("Registering JDeviceSettingsManagerFPD");
            _mDeviceSettingsManagerFPD->Register(mNotificationSink.baseInterface<Exchange::IDeviceSettingsManager::IFPD::INotification>());
            //Exchange::JDeviceSettingsManagerFPD::Register(*this, _mDeviceSettingsManagerFPD);
        } else {
            LOGERR("Failed to get IDeviceSettingsManager::IFPD interface");
        }

        LOGINFO("Trace - 3");
        _mDeviceSettingsManagerHDMIIn = service->Root<Exchange::IDeviceSettingsManager::IHDMIIn>(mConnectionId, RPC::CommunicationTimeOut, _T("DeviceSettingsManagerImp"));

        if (_mDeviceSettingsManagerHDMIIn != nullptr) {
            LOGINFO("Registering IDeviceSettingsManager::IHDMIIn");
            _mDeviceSettingsManagerHDMIIn->Register(mNotificationSink.baseInterface<Exchange::IDeviceSettingsManager::IHDMIIn::INotification>());
        }*/
        LOGINFO("Trace - 4");
        EXIT_LOG;

        // On success return empty, to indicate there is no error text.
        return (string());
    }

    void DeviceSettingsManager::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ENTRY_LOG;
        if (mService != nullptr) {
            ASSERT(mService == service);
            //mService->Unregister(&mNotificationSink);
            mService->Unregister(mNotificationSink.baseInterface<RPC::IRemoteConnection::INotification>());
            mService->Unregister(mNotificationSink.baseInterface<PluginHost::IShell::ICOMLink::INotification>());
            /*if (_mDeviceSettingsManagerFPD != nullptr) {
                _mDeviceSettingsManagerFPD->Unregister(&mNotificationSink);
            }*/
            /*if (_mDeviceSettingsManagerHDMIIn != nullptr) {
                _mDeviceSettingsManagerHDMIIn->Unregister(&mNotificationSink);
            }*/

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
        EXIT_LOG;
        // No additional info to report.
        return (string());
    }


    void DeviceSettingsManager::Deactivated(RPC::IRemoteConnection* connection)
    {
        ENTRY_LOG
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (mConnectionId == connection->Id()) {
            ASSERT(mService != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(mService, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
        EXIT_LOG;
    }

} // namespace Plugin
} // namespace WPEFramework
