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

#pragma once

#include "Module.h"

#include <interfaces/json/JDeviceSettingsManagerCompositeIn.h>
#include <interfaces/json/JDeviceSettingsManagerFPD.h>
#include <interfaces/json/JDeviceSettingsManagerVideoDevice.h>
#include <interfaces/json/JDeviceSettingsManagerDisplay.h>
#include <interfaces/json/JDeviceSettingsManagerHDMIIn.h>
#include <interfaces/json/JDeviceSettingsManagerHost.h>
#include <interfaces/json/JDeviceSettingsManagerVideoPort.h>
#include <interfaces/IDeviceSettingsManager.h>

#include "UtilsLogging.h"
#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#define ENTRY_LOG LOGINFO("%d: Enter %s \n", __LINE__, __func__);
#define EXIT_LOG LOGINFO("%d: EXIT %s \n", __LINE__, __func__);

/*using AudioPortType         = WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioPortType;
using AudioPortType         = WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioFormat;
using AudioPortType         = WPEFramework::Exchange::IDeviceSettingsManagerAudio::DolbyAtmosCapability;
using AudioPortType         = WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioPortState;*/
using FPDTimeFormat         = WPEFramework::Exchange::IDeviceSettingsManagerFPD::FPDTimeFormat;

namespace WPEFramework {
namespace Plugin {

    class DeviceSettingsManager : public PluginHost::IPlugin,
                                  public PluginHost::JSONRPC {
    private:
        class NotificationHandler
            : public RPC::IRemoteConnection::INotification
              , public PluginHost::IShell::ICOMLink::INotification
           // , public Exchange::IDeviceSettingsManagerCompositeIn::INotification
           // , public Exchange::IDeviceSettingsManagerAudio::INotification
            , public Exchange::IDeviceSettingsManagerFPD::INotification
           // , public Exchange::IDeviceSettingsManagerVideoDevice::INotification
           // , public Exchange::IDeviceSettingsManagerDisplay::INotification
           // , public Exchange::IDeviceSettingsManagerHDMIIn::INotification
           // , public Exchange::IDeviceSettingsManagerHost::INotification
           // , public Exchange::IDeviceSettingsManagerVideoPort::INotification
            {
        private:
            NotificationHandler()                                      = delete;
            NotificationHandler(const NotificationHandler&)            = delete;
            NotificationHandler& operator=(const NotificationHandler&) = delete;

        public:
            explicit NotificationHandler(DeviceSettingsManager* parent)
                : mParent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            virtual ~NotificationHandler()
            {
            }

            template <typename T>
            T* baseInterface()
            {
                static_assert(std::is_base_of<T, NotificationHandler>(), "base type mismatch");
                return static_cast<T*>(this);
            }

            BEGIN_INTERFACE_MAP(NotificationHandler)
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManagerCompositeIn::INotification)
            INTERFACE_ENTRY(Exchange::IDeviceSettingsManagerFPD::INotification)
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManagerVideoDevice::INotification)
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManagerDisplay::INotification)
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManagerHDMIIn::INotification)
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManagerHost::INotification)
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManagerVideoPort::INotification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

            void Activated(RPC::IRemoteConnection*) override
            {
            }

            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                mParent.Deactivated(connection);
            }

            void Dangling(const Core::IUnknown* remote, const uint32_t interfaceId) override
            {
                ASSERT(remote != nullptr);
                mParent.CallbackRevoked(remote, interfaceId);
            }

            void Revoked(const Core::IUnknown* remote, const uint32_t interfaceId) override
            {
                ASSERT(remote != nullptr);
                mParent.CallbackRevoked(remote, interfaceId);
            }

            virtual void OnFPDTimeFormatChanged(const FPDTimeFormat timeFormat) override
            {
                LOGINFO("OnFPDTimeFormatChanged: timeFormat %d", timeFormat);
                Exchange::JDeviceSettingsManagerFPD::Event::OnFPDTimeFormatChanged(mParent, timeFormat);
            }

        private:
            DeviceSettingsManager& mParent;
        };
    public:
        DeviceSettingsManager(const DeviceSettingsManager&) = delete;
        DeviceSettingsManager(DeviceSettingsManager&&) = delete;
        DeviceSettingsManager& operator=(const DeviceSettingsManager&) = delete;
        DeviceSettingsManager& operator=(DeviceSettingsManager&) = delete;

        DeviceSettingsManager();
        virtual ~DeviceSettingsManager();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(DeviceSettingsManager)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManagerCompositeIn, _mDeviceSettingsManagerCompositeIn)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManagerAudio, _mDeviceSettingsManagerAudio)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManagerFPD, _mDeviceSettingsManagerFPD)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManagerVideoDevice, _mDeviceSettingsManagerVideoDevice)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManagerDisplay, _mDeviceSettingsManagerDisplay)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManagerHDMIIn, _mDeviceSettingsManagerHDMIIn)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManagerHost, _mDeviceSettingsManagerHost)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManagerVideoPort, _mDeviceSettingsManagerVideoPort)
        END_INTERFACE_MAP

    public:

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;


    private:
        void Deactivated(RPC::IRemoteConnection* connection);
        void CallbackRevoked(const Core::IUnknown* remote, const uint32_t interfaceId);

    private:
        uint32_t mConnectionId;
        PluginHost::IShell* mService;
        Exchange::IDeviceSettingsManagerCompositeIn* _mDeviceSettingsManagerCompositeIn;
        Exchange::IDeviceSettingsManagerAudio* _mDeviceSettingsManagerAudio;
        Exchange::IDeviceSettingsManagerFPD* _mDeviceSettingsManagerFPD;
        Exchange::IDeviceSettingsManagerVideoDevice* _mDeviceSettingsManagerVideoDevice;
        Exchange::IDeviceSettingsManagerDisplay* _mDeviceSettingsManagerDisplay;
        Exchange::IDeviceSettingsManagerHDMIIn* _mDeviceSettingsManagerHDMIIn;
        Exchange::IDeviceSettingsManagerHost* _mDeviceSettingsManagerHost;
        Exchange::IDeviceSettingsManagerVideoPort* _mDeviceSettingsManagerVideoPort;
        Core::Sink<NotificationHandler> mNotificationSink;

    };

} // namespace Plugin
} // namespace WPEFramework
