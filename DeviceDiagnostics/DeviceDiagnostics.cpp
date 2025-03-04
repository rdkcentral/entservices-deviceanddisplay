/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
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

#include "DeviceDiagnostics.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 1
#define API_VERSION_NUMBER_PATCH 2

namespace WPEFramework
{

    namespace {

        static Plugin::Metadata<Plugin::DeviceDiagnostics> metadata(
            // Version (Major, Minor, Patch)
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    namespace Plugin
    {

    /*
     *Register DeviceDiagnostics module as wpeframework plugin
     **/
    SERVICE_REGISTRATION(DeviceDiagnostics, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    DeviceDiagnostics::DeviceDiagnostics() : _service(nullptr), _connectionId(0), _deviceDiagnostics(nullptr), _deviceDiagnosticsNotification(this)
    {
        SYSLOG(Logging::Startup, (_T("DeviceDiagnostics Constructor")));
    }

    DeviceDiagnostics::~DeviceDiagnostics()
    {
        SYSLOG(Logging::Shutdown, (string(_T("DeviceDiagnostics Destructor"))));
    }

    const string DeviceDiagnostics::Initialize(PluginHost::IShell* service)
    {
        string message="";

        ASSERT(nullptr != service);
        ASSERT(nullptr == _service);
        ASSERT(nullptr == _deviceDiagnostics);
        ASSERT(0 == _connectionId);

        SYSLOG(Logging::Startup, (_T("DeviceDiagnostics::Initialize: PID=%u"), getpid()));

        _service = service;
        _service->AddRef();
        _service->Register(&_deviceDiagnosticsNotification);

        _deviceDiagnostics = service->Root<Exchange::IDeviceDiagnostics>(_connectionId, 5000, _T("DeviceDiagnosticsImplementation"));

        if(nullptr != _deviceDiagnostics)
        {
            // Register for notifications
            _deviceDiagnostics->Register(&_deviceDiagnosticsNotification);
            // Invoking Plugin API register to wpeframework
            Exchange::JDeviceDiagnostics::Register(*this, _deviceDiagnostics);
        }
        else
        {
            SYSLOG(Logging::Startup, (_T("DeviceDiagnostics::Initialize: Failed to initialise DeviceDiagnostics plugin")));
            message = _T("DeviceDiagnostics plugin could not be initialised");
        }
        
        return message;
    }

    void DeviceDiagnostics::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        SYSLOG(Logging::Shutdown, (string(_T("DeviceDiagnostics::Deinitialize"))));

        // Make sure the Activated and Deactivated are no longer called before we start cleaning up..
        _service->Unregister(&_deviceDiagnosticsNotification);

        if (nullptr != _deviceDiagnostics)
        {

            _deviceDiagnostics->Unregister(&_deviceDiagnosticsNotification);
            Exchange::JDeviceDiagnostics::Unregister(*this);

            // Stop processing:
            RPC::IRemoteConnection* connection = service->RemoteConnection(_connectionId);
            VARIABLE_IS_NOT_USED uint32_t result = _deviceDiagnostics->Release();

            _deviceDiagnostics = nullptr;

            // It should have been the last reference we are releasing,
            // so it should endup in a DESTRUCTION_SUCCEEDED, if not we
            // are leaking...
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

            // If this was running in a (container) process...
            if (nullptr != connection)
            {
               // Lets trigger the cleanup sequence for
               // out-of-process code. Which will guard
               // that unwilling processes, get shot if
               // not stopped friendly :-)
               try
               {
                   connection->Terminate();
                   // Log success if needed
                   LOGWARN("Connection terminated successfully.");
               }
               catch (const std::exception& e)
               {
                   std::string errorMessage = "Failed to terminate connection: ";
                   errorMessage += e.what();
                   LOGWARN("%s",errorMessage.c_str());
               }

               connection->Release();
            }
        }

        _connectionId = 0;
        _service->Release();
        _service = nullptr;
        SYSLOG(Logging::Shutdown, (string(_T("DeviceDiagnostics de-initialised"))));
    }

    string DeviceDiagnostics::Information() const
    {
       return ("This DeviceDiagnostics Plugin provides additional diagnostics information which includes device configuration and AV decoder status.");
    }

    void DeviceDiagnostics::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(nullptr != _service);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
} // namespace Plugin
} // namespace WPEFramework
