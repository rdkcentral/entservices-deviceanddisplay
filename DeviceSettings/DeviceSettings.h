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

//#include <interfaces/IDeviceSettingsManager.h>
#include <interfaces/IDeviceSettingsAudio.h>
#include <interfaces/IDeviceSettingsCompositeIn.h>
#include <interfaces/IDeviceSettingsDisplay.h>
#include <interfaces/IDeviceSettingsFPD.h>
#include <interfaces/IDeviceSettingsHDMIIn.h>
#include <interfaces/IDeviceSettingsHost.h>
//#include <interfaces/IDeviceSettingsManager.h>
#include <interfaces/IDeviceSettingsVideoDevice.h>
#include <interfaces/IDeviceSettingsVideoPort.h>

#include "UtilsLogging.h"
#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#include "DeviceSettingsTypes.h"


namespace WPEFramework {
namespace Plugin {

    class DeviceSettings : public PluginHost::IPlugin
    {
    private:
        class NotificationHandler : public RPC::IRemoteConnection::INotification
                                  , public PluginHost::IShell::ICOMLink::INotification
                                //  , public DeviceSettingsCompositeIn::INotification
                                //  , public DeviceSettingsAudio::INotification
                                  , public DeviceSettingsFPD::INotification
                                // , public DeviceSettingsVideoDevice::INotification
                                // , public DeviceSettingsDisplay::INotification
                                  , public DeviceSettingsHDMIIn::INotification
                                // , public DeviceSettingsHost::INotification
                                // , public DeviceSettingsVideoPort::INotification
            {
        private:
            NotificationHandler()                                      = delete;
            NotificationHandler(const NotificationHandler&)            = delete;
            NotificationHandler& operator=(const NotificationHandler&) = delete;

        public:
            explicit NotificationHandler(DeviceSettings* parent)
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
            //INTERFACE_ENTRY(DeviceSettingsCompositeIn::INotification)
            INTERFACE_ENTRY(DeviceSettingsFPD::INotification)
            //INTERFACE_ENTRY(DeviceSettingsVideoDevice::INotification)
            //INTERFACE_ENTRY(DeviceSettingsDisplay::INotification)
            INTERFACE_ENTRY(DeviceSettingsHDMIIn::INotification)
            //INTERFACE_ENTRY(DeviceSettingsHost::INotification)
            //INTERFACE_ENTRY(DeviceSettingsVideoPort::INotification)
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
            DeviceSettings& mParent;
        };
    public:
        DeviceSettings(const DeviceSettings&) = delete;
        DeviceSettings(DeviceSettings&&) = delete;
        DeviceSettings& operator=(const DeviceSettings&) = delete;
        DeviceSettings& operator=(DeviceSettings&) = delete;

        DeviceSettings();
        virtual ~DeviceSettings();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(DeviceSettings)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            //INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(DeviceSetting, _mDeviceSettings)
            //INTERFACE_AGGREGATE(DeviceSettingsCompositeIn, _mDeviceSettingsCompositeIn)
            //INTERFACE_AGGREGATE(DeviceSettingsAudio, _mDeviceSettingsAudio)
            INTERFACE_AGGREGATE(DeviceSettingsFPD, _mDeviceSettingsFPD)
            //INTERFACE_AGGREGATE(DeviceSettingsVideoDevice, _mDeviceSettingsVideoDevice)
            //INTERFACE_AGGREGATE(DeviceSettingsDisplay, _mDeviceSettingsDisplay)
            INTERFACE_AGGREGATE(DeviceSettingsHDMIIn, _mDeviceSettingsHDMIIn)
            //INTERFACE_AGGREGATE(DeviceSettingsHost, _mDeviceSettingsHost)
            //INTERFACE_AGGREGATE(DeviceSettingsVideoPort, _mDeviceSettingsVideoPort)
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
        DeviceSetting* _mDeviceSettings;
        DeviceSettingsCompositeIn* _mDeviceSettingsCompositeIn;
        DeviceSettingsAudio* _mDeviceSettingsAudio;
        DeviceSettingsFPD* _mDeviceSettingsFPD;
        DeviceSettingsVideoDevice* _mDeviceSettingsVideoDevice;
        DeviceSettingsDisplay* _mDeviceSettingsDisplay;
        DeviceSettingsHDMIIn* _mDeviceSettingsHDMIIn;
        DeviceSettingsHost* _mDeviceSettingsHost;
        DeviceSettingsVideoPort* _mDeviceSettingsVideoPort;
        Core::Sink<NotificationHandler> mNotificationSink;

    };

} // namespace Plugin
} // namespace WPEFramework
