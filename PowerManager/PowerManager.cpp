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

#include "PowerManager.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework
{

    namespace {

        static Plugin::Metadata<Plugin::PowerManager> metadata(
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
     *Register PowerManager module as wpeframework plugin
     **/
    SERVICE_REGISTRATION(PowerManager, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    PowerManager::PowerManager() : _service(nullptr), _connectionId(0), _powerManager(nullptr), _powermanagersNotification(this)
    {
        SYSLOG(Logging::Startup, (_T("PowerManager Constructor")));
    }

    PowerManager::~PowerManager()
    {
        SYSLOG(Logging::Shutdown, (string(_T("PowerManager Destructor"))));
    }

    const string PowerManager::Initialize(PluginHost::IShell* service)
    {
        string message="";

        ASSERT(nullptr != service);
        ASSERT(nullptr == _service);
        ASSERT(nullptr == _powerManager);
        ASSERT(0 == _connectionId);

        SYSLOG(Logging::Startup, (_T("PowerManager::Initialize: PID=%u"), getpid()));

        _service = service;
        _service->AddRef();
        _service->Register(&_powermanagersNotification);
        _powerManager = _service->Root<Exchange::IPowerManager>(_connectionId, 5000, _T("PowerManagerImplementation"));

        if(nullptr != _powerManager)
        {
            // Register for notifications
            _powerManager->Register(&_powermanagersNotification);
            // Invoking Plugin API register to wpeframework
            Exchange::JPowerManager::Register(*this, _powerManager);
        }
        else
        {
            SYSLOG(Logging::Startup, (_T("PowerManager::Initialize: Failed to initialise PowerManager plugin")));
            message = _T("PowerManager plugin could not be initialised");
        }

        if (0 != message.length())
        {
           Deinitialize(service);
        }

        return message;
    }

    void PowerManager::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        SYSLOG(Logging::Shutdown, (string(_T("PowerManager::Deinitialize"))));

        // Make sure the Activated and Deactivated are no longer called before we start cleaning up..
        _service->Unregister(&_powermanagersNotification);

        if (nullptr != _powerManager)
        {

            _powerManager->Unregister(&_powermanagersNotification);
            Exchange::JPowerManager::Unregister(*this);

            // Stop processing:
            RPC::IRemoteConnection* connection = service->RemoteConnection(_connectionId);
            VARIABLE_IS_NOT_USED uint32_t result = _powerManager->Release();

            _powerManager = nullptr;

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
        SYSLOG(Logging::Shutdown, (string(_T("PowerManager de-initialised"))));
    }

    string PowerManager::Information() const
    {
       return ("This PowerManager Plugin facilitates communication between Thunder plugins and the PwrMgr IARM manager");
    }

    void PowerManager::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(nullptr != _service);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
} // namespace Plugin
} // namespace WPEFramework
