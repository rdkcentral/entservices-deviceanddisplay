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


#include <interfaces/IDeviceSettingsManager.h>

#include "UtilsLogging.h"
#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#define ENTRY_LOG LOGINFO("%d: Enter %s \n", __LINE__, __func__);
#define EXIT_LOG LOGINFO("%d: EXIT %s \n", __LINE__, __func__);

namespace WPEFramework {
namespace Plugin {

    class DeviceSettingsManager : public PluginHost::IPlugin
    {
        using HDMIInPort               = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort;
        using HDMIInSignalStatus       = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInSignalStatus;
        using HDMIVideoPortResolution  = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIVideoPortResolution;
        using HDMIInAviContentType     = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInAviContentType;
        using HDMIInVRRType            = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInVRRType;
        using FPDTimeFormat            = WPEFramework::Exchange::IDeviceSettingsManager::IFPD::FPDTimeFormat;

    private:
        class NotificationHandler
            : public RPC::IRemoteConnection::INotification
              , public PluginHost::IShell::ICOMLink::INotification
           // , public Exchange::IDeviceSettingsManager::ICompositeIn::INotification
           // , public Exchange::IDeviceSettingsManager::IAudio::INotification
            , public Exchange::IDeviceSettingsManager::IFPD::INotification
           // , public Exchange::IDeviceSettingsManager::IVideoDevice::INotification
           // , public Exchange::IDeviceSettingsManager::IDisplay::INotification
              , public Exchange::IDeviceSettingsManager::IHDMIIn::INotification
           // , public Exchange::IDeviceSettingsManager::IHost::INotification
           // , public Exchange::IDeviceSettingsManager::IVideoPort::INotification
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
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManager::ICompositeIn::INotification)
            INTERFACE_ENTRY(Exchange::IDeviceSettingsManager::IFPD::INotification)
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManager::IVideoDevice::INotification)
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManager::IDisplay::INotification)
            INTERFACE_ENTRY(Exchange::IDeviceSettingsManager::IHDMIIn::INotification)
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManager::IHost::INotification)
            //INTERFACE_ENTRY(Exchange::IDeviceSettingsManager::IVideoPort::INotification)
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

            void OnFPDTimeFormatChanged(const FPDTimeFormat timeFormat) override
            {
                LOGINFO("OnFPDTimeFormatChanged: timeFormat %d", timeFormat);
                //Exchange::JDeviceSettingsManagerFPD::Event::OnFPDTimeFormatChanged(mParent, timeFormat);
            }

            void OnHDMIInEventHotPlug(const HDMIInPort port, const bool isConnected) override 
            {
                LOGINFO("OnHDMIInEventHotPlug:");
            }

            void OnHDMIInEventSignalStatus(const HDMIInPort port, const HDMIInSignalStatus signalStatus) override
            {
                LOGINFO("OnHDMIInEventSignalStatus");
            }

            void OnHDMIInEventStatus(const HDMIInPort activePort, const bool isPresented) override
            {
                LOGINFO("OnHDMIInEventStatus");
            }

            void OnHDMIInVideoModeUpdate(const HDMIInPort port, const HDMIVideoPortResolution videoPortResolution) override
            {
                LOGINFO("OnHDMIInVideoModeUpdate");
            }

            void OnHDMIInAllmStatus(const HDMIInPort port, const bool allmStatus) override
            {
                LOGINFO("OnHDMIInAllmStatus");
            }

            void OnHDMIInAVIContentType(const HDMIInPort port, const HDMIInAviContentType aviContentType) override
            {
                LOGINFO("OnHDMIInAVIContentType");
            }

            void OnHDMIInAVLatency(const int32_t audioDelay, const int32_t videoDelay) override
            {
                LOGINFO("OnHDMIInAVLatency");
            }

            void OnHDMIInVRRStatus(const HDMIInPort port, const HDMIInVRRType vrrType) override
            {
                LOGINFO("OnHDMIInVRRStatus");
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
            //INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManager, _mDeviceSettingsManager)
            //INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManager::ICompositeIn, _mDeviceSettingsManagerCompositeIn)
            //INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManager::IAudio, _mDeviceSettingsManagerAudio)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManager::IFPD, _mDeviceSettingsManagerFPD)
            //INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManager::IVideoDevice, _mDeviceSettingsManagerVideoDevice)
            //INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManager::IDisplay, _mDeviceSettingsManagerDisplay)
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManager::IHDMIIn, _mDeviceSettingsManagerHDMIIn)
            //INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManager::IHost, _mDeviceSettingsManagerHost)
            //INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManager::IVideoPort, _mDeviceSettingsManagerVideoPort)
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
        Exchange::IDeviceSettingsManager* _mDeviceSettingsManager;
        Exchange::IDeviceSettingsManager::ICompositeIn* _mDeviceSettingsManagerCompositeIn;
        Exchange::IDeviceSettingsManager::IAudio* _mDeviceSettingsManagerAudio;
        Exchange::IDeviceSettingsManager::IFPD* _mDeviceSettingsManagerFPD;
        Exchange::IDeviceSettingsManager::IVideoDevice* _mDeviceSettingsManagerVideoDevice;
        Exchange::IDeviceSettingsManager::IDisplay* _mDeviceSettingsManagerDisplay;
        Exchange::IDeviceSettingsManager::IHDMIIn* _mDeviceSettingsManagerHDMIIn;
        Exchange::IDeviceSettingsManager::IHost* _mDeviceSettingsManagerHost;
        Exchange::IDeviceSettingsManager::IVideoPort* _mDeviceSettingsManagerVideoPort;
        Core::Sink<NotificationHandler> mNotificationSink;

    };

} // namespace Plugin
} // namespace WPEFramework
