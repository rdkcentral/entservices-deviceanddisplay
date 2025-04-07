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

#pragma once

#include "Module.h"
#include <interfaces/IDeviceDiagnostics.h>
#include <interfaces/json/JDeviceDiagnostics.h>
#include <interfaces/json/JsonData_DeviceDiagnostics.h>
#include "UtilsLogging.h"
#include "tracing/Logging.h"

namespace WPEFramework
{
    namespace Plugin
    {
        class DeviceDiagnostics : public PluginHost::IPlugin, public PluginHost::JSONRPC 
        {
            private:
                class Notification : public RPC::IRemoteConnection::INotification, public Exchange::IDeviceDiagnostics::INotification
                {
                    private:
                        Notification() = delete;
                        Notification(const Notification&) = delete;
                        Notification& operator=(const Notification&) = delete;

                    public:
                    explicit Notification(DeviceDiagnostics* parent) 
                        : _parent(*parent)
                        {
                            ASSERT(parent != nullptr);
                        }

                        virtual ~Notification()
                        {
                        }

                        BEGIN_INTERFACE_MAP(Notification)
                        INTERFACE_ENTRY(Exchange::IDeviceDiagnostics::INotification)
                        INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                        END_INTERFACE_MAP

                        void Activated(RPC::IRemoteConnection*) override
                        {
                        }

                        void Deactivated(RPC::IRemoteConnection *connection) override
                        {
                            _parent.Deactivated(connection);
                        }

                        void OnAVDecoderStatusChanged(const string& AVDecoderStatus ) override
                        {
                            LOGINFO("OnAVDecoderStatusChanged: AVDecoderStatus %s\n", AVDecoderStatus.c_str());
                            Exchange::JDeviceDiagnostics::Event::OnAVDecoderStatusChanged(_parent, AVDecoderStatus);
                        }

                    private:
                        DeviceDiagnostics& _parent;
                };

                public:
                    DeviceDiagnostics(const DeviceDiagnostics&) = delete;
                    DeviceDiagnostics& operator=(const DeviceDiagnostics&) = delete;

                    DeviceDiagnostics();
                    virtual ~DeviceDiagnostics();

                    BEGIN_INTERFACE_MAP(DeviceDiagnostics)
                    INTERFACE_ENTRY(PluginHost::IPlugin)
                    INTERFACE_ENTRY(PluginHost::IDispatcher)
                    INTERFACE_AGGREGATE(Exchange::IDeviceDiagnostics, _deviceDiagnostics)
                    END_INTERFACE_MAP

                    //  IPlugin methods
                    // -------------------------------------------------------------------------------------------------------
                    const string Initialize(PluginHost::IShell* service) override;
                    void Deinitialize(PluginHost::IShell* service) override;
                    string Information() const override;

                private:
                    void Deactivated(RPC::IRemoteConnection* connection);

                private:
                    PluginHost::IShell* _service{};
                    uint32_t _connectionId{};
                    Exchange::IDeviceDiagnostics* _deviceDiagnostics{};
                    Core::Sink<Notification> _deviceDiagnosticsNotification;
       };
    } // namespace Plugin
} // namespace WPEFramework
