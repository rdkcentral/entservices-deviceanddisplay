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

#include "DeviceSettingsManagerTypes.h"


namespace WPEFramework {
namespace Plugin {

    class DeviceSettingsManager : public PluginHost::IPlugin
    {
    private:
        class NotificationHandler : public RPC::IRemoteConnection::INotification
                                  , public PluginHost::IShell::ICOMLink::INotification
                                //  , public DeviceSettingsManagerCompositeIn::INotification
                                //  , public DeviceSettingsManagerAudio::INotification
                                  , public DeviceSettingsManagerFPD::INotification
                                // , public DeviceSettingsManagerVideoDevice::INotification
                                // , public DeviceSettingsManagerDisplay::INotification
                                  , public DeviceSettingsManagerHDMIIn::INotification
                                // , public DeviceSettingsManagerHost::INotification
                                // , public DeviceSettingsManagerVideoPort::INotification
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
            //INTERFACE_ENTRY(DeviceSettingsManagerCompositeIn::INotification)
            INTERFACE_ENTRY(DeviceSettingsManagerFPD::INotification)
            //INTERFACE_ENTRY(DeviceSettingsManagerVideoDevice::INotification)
            //INTERFACE_ENTRY(DeviceSettingsManagerDisplay::INotification)
            INTERFACE_ENTRY(DeviceSettingsManagerHDMIIn::INotification)
            //INTERFACE_ENTRY(DeviceSettingsManagerHost::INotification)
            //INTERFACE_ENTRY(DeviceSettingsManagerVideoPort::INotification)
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
#ifndef USE_LEGACY_INTERFACE
            INTERFACE_AGGREGATE(Exchange::IDeviceSettingsManager, _mDeviceSettingsManager)
#endif
            //INTERFACE_AGGREGATE(DeviceSettingsManagerCompositeIn, _mDeviceSettingsManagerCompositeIn)
            //INTERFACE_AGGREGATE(DeviceSettingsManagerAudio, _mDeviceSettingsManagerAudio)
            INTERFACE_AGGREGATE(DeviceSettingsManagerFPD, _mDeviceSettingsManagerFPD)
            //INTERFACE_AGGREGATE(DeviceSettingsManagerVideoDevice, _mDeviceSettingsManagerVideoDevice)
            //INTERFACE_AGGREGATE(DeviceSettingsManagerDisplay, _mDeviceSettingsManagerDisplay)
            INTERFACE_AGGREGATE(DeviceSettingsManagerHDMIIn, _mDeviceSettingsManagerHDMIIn)
            //INTERFACE_AGGREGATE(DeviceSettingsManagerHost, _mDeviceSettingsManagerHost)
            //INTERFACE_AGGREGATE(DeviceSettingsManagerVideoPort, _mDeviceSettingsManagerVideoPort)
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
#ifndef USE_LEGACY_INTERFACE
        Exchange::IDeviceSettingsManager* _mDeviceSettingsManager;
#endif
        DeviceSettingsManagerCompositeIn* _mDeviceSettingsManagerCompositeIn;
        DeviceSettingsManagerAudio* _mDeviceSettingsManagerAudio;
        DeviceSettingsManagerFPD* _mDeviceSettingsManagerFPD;
        DeviceSettingsManagerVideoDevice* _mDeviceSettingsManagerVideoDevice;
        DeviceSettingsManagerDisplay* _mDeviceSettingsManagerDisplay;
        DeviceSettingsManagerHDMIIn* _mDeviceSettingsManagerHDMIIn;
        DeviceSettingsManagerHost* _mDeviceSettingsManagerHost;
        DeviceSettingsManagerVideoPort* _mDeviceSettingsManagerVideoPort;
        Core::Sink<NotificationHandler> mNotificationSink;

    };

} // namespace Plugin
} // namespace WPEFramework
