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

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
//#include "libIBus.h"
#include <string.h> 
#include <iostream>
#include <fstream>

#include "DeviceSettings.h"
#include <interfaces/IConfiguration.h>


namespace WPEFramework {

namespace Plugin
{
    SERVICE_REGISTRATION(DeviceSettings, API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);

    namespace {
        static Metadata<DeviceSettings> metadata(
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

    DeviceSettings::DeviceSettings()
        : mConnectionId(0)
        , mService(nullptr)
#ifndef USE_LEGACY_INTERFACE
        , _mDeviceSettings(nullptr)
#endif
        , mNotificationSink(this)

    {
        #if (defined(RDK_LOGGER_ENABLED) || defined(DSMGR_LOGGER_ENABLED))

        const char* PdebugConfigFile = NULL;
        const char* DSMGR_DEBUG_ACTUAL_PATH    = "/etc/debug.ini";
        const char* DSMGR_DEBUG_OVERRIDE_PATH  = "/opt/debug.ini";

        /* Init the logger */
        if (access(DSMGR_DEBUG_OVERRIDE_PATH, F_OK) != -1 ) {
            PdebugConfigFile = DSMGR_DEBUG_OVERRIDE_PATH;
        }
        else {
            PdebugConfigFile = DSMGR_DEBUG_ACTUAL_PATH;
        }

        if (rdk_logger_init(PdebugConfigFile) == 0) {
            b_rdk_logger_enabled = 1;
        }

#endif

    }

    
    DeviceSettings::~DeviceSettings()
    {
    }
    const string DeviceSettings::Initialize(PluginHost::IShell * service)
    {
        string message = "";
        ENTRY_LOG;
        ASSERT(service != nullptr);
        ASSERT(mService == nullptr);
        ASSERT(mConnectionId == 0);
#ifdef USE_LEGACY_INTERFACE
        ASSERT(_mDeviceSettingsFPD == nullptr);
        ASSERT(_mDeviceSettingsHDMIIn == nullptr);
#else
        ASSERT(_mDeviceSettings == nullptr);
#endif
        mService = service;
        mService->AddRef();

        mService->Register(mNotificationSink.baseInterface<RPC::IRemoteConnection::INotification>());
        mService->Register(mNotificationSink.baseInterface<PluginHost::IShell::ICOMLink::INotification>());

#ifdef USE_LEGACY_INTERFACE
        // Get IDeviceSettingsFPD interface.
        _mDeviceSettingsFPD = service->Root<Exchange::IDeviceSettingsFPD>(mConnectionId, RPC::CommunicationTimeOut, _T("DeviceSettingsFPDImpl"));


        if (_mDeviceSettingsFPD == nullptr) {
            SYSLOG(Logging::Startup, (_T("DeviceSettings::Initialize: Failed to initialise DeviceSettings plugin")));
            message = _T("DeviceSettings plugin could not be initialised");
            LOGERR("Failed to get IDeviceSettingsFPD interface");
        } else {
            LOGINFO("DeviceSettingsFPD interface obtained successfully");
            #if 0
            // Use QueryInterface to get HDMI interface from the same object instance
            _mDeviceSettingsHDMIIn = _mDeviceSettingsFPD->QueryInterface<Exchange::IDeviceSettingsHDMIIn>();

            if (_mDeviceSettingsHDMIIn == nullptr) {
                SYSLOG(Logging::Startup, (_T("DeviceSettings::Initialize: Failed to query HDMI interface from same object")));
                message = _T("DeviceSettings HDMI interface could not be queried");
                LOGERR("Failed to QueryInterface for IDeviceSettingsHDMIIn from same object");
            } else {
                LOGINFO("DeviceSettingsHDMIIn interface queried successfully");
            }
            #endif
        }
        // Get IDeviceSettingsHDMIIn interface from 
        _mDeviceSettingsHDMIIn = service->Root<Exchange::IDeviceSettingsHDMIIn>(mConnectionId, RPC::CommunicationTimeOut, _T("DeviceSettingsHdmiInImp"));

        if (_mDeviceSettingsHDMIIn == nullptr) {
            SYSLOG(Logging::Startup, (_T("DeviceSettings::Initialize: Failed to get IDeviceSettingsHDMIIn interface")));
            message = _T("DeviceSettings HDMI interface could not be queried");
            LOGERR("Failed to get IDeviceSettingsHDMIIn interface");
        } else {
            LOGINFO("DeviceSettingsHDMIIn interface queried successfully");
        }
#else
        // Get the unified interface that provides both FPD and HDMI functionality
        _mDeviceSettings = service->Root<Exchange::IDeviceSettings>(mConnectionId, RPC::CommunicationTimeOut, _T("DeviceSettingsImp"));

        if (_mDeviceSettings == nullptr) {
            SYSLOG(Logging::Startup, (_T("DeviceSettings::Initialize: Failed to initialise DeviceSettings plugin")));
            message = _T("DeviceSettings plugin could not be initialised");
            LOGERR("Failed to get IDeviceSettings interface");
        } else {
            LOGINFO("DeviceSettingsImp initialized successfully");
        }
#endif
        if (0 != message.length()) {
            Deinitialize(service);
        }
        EXIT_LOG;

        // On success return empty, to indicate there is no error text.
        return (message);
    }

    void DeviceSettings::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ENTRY_LOG;
        if (mService != nullptr) {
            ASSERT(mService == service);
            mService->Unregister(mNotificationSink.baseInterface<RPC::IRemoteConnection::INotification>());
            mService->Unregister(mNotificationSink.baseInterface<PluginHost::IShell::ICOMLink::INotification>());
#ifdef USE_LEGACY_INTERFACE
            _mDeviceSettingsFPD = nullptr;
            _mDeviceSettingsHDMIIn = nullptr;
#else
            _mDeviceSettings = nullptr;
#endif
            mService->Release();
            mService = nullptr;
            mConnectionId = 0;
            SYSLOG(Logging::Shutdown, (string(_T("DeviceSettings de-initialised"))));
        }
        EXIT_LOG;
    }

    string DeviceSettings::Information() const
    {
        ENTRY_LOG;
        // No additional info to report.
        EXIT_LOG;

        return (string());
    }

    void DeviceSettings::Deactivated(RPC::IRemoteConnection* connection)
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

    void DeviceSettings::CallbackRevoked(const Core::IUnknown* remote, const uint32_t interfaceId)
    {
        // Add your handling code here, or leave empty if not needed
        LOGINFO("CallbackRevoked called for interfaceId %u", interfaceId);
    }

} // namespace Plugin
} // namespace WPEFramework
