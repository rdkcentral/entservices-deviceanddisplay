/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#ifndef USERPLUGIN_USERPLUGIN_H
#define USERPLUGIN_USERPLUGIN_H

#include "Module.h"

#include "tracing/Logging.h"
#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#include <interfaces/IUserPlugin.h>
#include <interfaces/json/JUserPlugin.h>
#include <interfaces/IPowerManager.h>
#include <interfaces/IDeviceSettingsManager.h>

#include "PowerManagerInterface.h"
using namespace WPEFramework::Exchange;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

namespace WPEFramework
{
    namespace Plugin
    {
        class UserPlugin : public PluginHost::IPlugin, public PluginHost::JSONRPC, public Exchange::IUserPlugin 
        {
        private:

            class PowerManagerNotification : public virtual Exchange::IPowerManager::IModeChangedNotification {
            private:
                PowerManagerNotification(const PowerManagerNotification&) = delete;
                PowerManagerNotification& operator=(const PowerManagerNotification&) = delete;
            
            public:
                explicit PowerManagerNotification(UserPlugin& parent)
                    : _parent(parent)
                {
                }

            public:
                void OnPowerModeChanged(const PowerState currentState, const PowerState newState) override
                {
                    _parent.OnPowerModeChanged(currentState, newState);
                }

                template <typename T>
                T* baseInterface()
                {
                    static_assert(std::is_base_of<T, PowerManagerNotification>(), "base type mismatch");
                    return static_cast<T*>(this);
                }

                BEGIN_INTERFACE_MAP(PowerManagerNotification)
                INTERFACE_ENTRY(Exchange::IPowerManager::IModeChangedNotification)
                END_INTERFACE_MAP
            
            private:
                UserPlugin& _parent;
            };

            class HDMIInNotification : public virtual Exchange::IDeviceSettingsManager::IHDMIIn::INotification {
            private:
                HDMIInNotification(const HDMIInNotification&) = delete;
                HDMIInNotification& operator=(const HDMIInNotification&) = delete;
            
            public:
                explicit HDMIInNotification(UserPlugin& parent)
                    : _parent(parent)
                {
                }

            public:
                void OnHDMIInEventHotPlug(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const bool isConnected) override
                {
                    _parent.OnHDMIInEventHotPlug(port, isConnected);
                }

                void OnHDMIInEventSignalStatus(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInSignalStatus signalStatus) override
                {
                    _parent.OnHDMIInEventSignalStatus(port, signalStatus);
                }

                void OnHDMIInEventStatus(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort activePort, const bool isPresented) override
                {
                    _parent.OnHDMIInEventStatus(activePort, isPresented);
                }

                void OnHDMIInVideoModeUpdate(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIVideoPortResolution videoPortResolution) override
                {
                    _parent.OnHDMIInVideoModeUpdate(port, videoPortResolution);
                }

                void OnHDMIInAllmStatus(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const bool allmStatus) override
                {
                    _parent.OnHDMIInAllmStatus(port, allmStatus);
                }

                void OnHDMIInAVIContentType(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInAviContentType aviContentType) override
                {
                    _parent.OnHDMIInAVIContentType(port, aviContentType);
                }

                void OnHDMIInAVLatency(const int32_t audioDelay, const int32_t videoDelay) override
                {
                    _parent.OnHDMIInAVLatency(audioDelay, videoDelay);
                }

                void OnHDMIInVRRStatus(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInVRRType vrrType) override
                {
                    _parent.OnHDMIInVRRStatus(port, vrrType);
                }

                template <typename T>
                T* baseInterface()
                {
                    static_assert(std::is_base_of<T, HDMIInNotification>(), "base type mismatch");
                    return static_cast<T*>(this);
                }

                BEGIN_INTERFACE_MAP(HDMIInNotification)
                INTERFACE_ENTRY(Exchange::IDeviceSettingsManager::IHDMIIn::INotification)
                END_INTERFACE_MAP
            
            private:
                UserPlugin& _parent;
            };

        public:
            static UserPlugin* _instance;
            // We do not allow this plugin to be copied !!
            UserPlugin(const UserPlugin &) = delete;
            UserPlugin &operator=(const UserPlugin &) = delete;

            UserPlugin();
            ~UserPlugin() override;

            BEGIN_INTERFACE_MAP(UserPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IUserPlugin)
            END_INTERFACE_MAP

            Core::hresult GetDevicePowerState(std::string& powerState) const;
            Core::hresult GetVolumeLevel (const string& port, string& level) const;
            void OnPowerModeChanged(const PowerState currentState, const PowerState newState);
            void onPowerStateChanged(string currentPowerState, string powerState);
            void TestSpecificHDMIInAPIs();
            void TestFPDAPIs();
            
            // HDMI In Event Handlers
            void OnHDMIInEventHotPlug(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const bool isConnected);
            void OnHDMIInEventSignalStatus(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInSignalStatus signalStatus);
            void OnHDMIInEventStatus(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort activePort, const bool isPresented);
            void OnHDMIInVideoModeUpdate(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIVideoPortResolution videoPortResolution);
            void OnHDMIInAllmStatus(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const bool allmStatus);
            void OnHDMIInAVIContentType(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInAviContentType aviContentType);
            void OnHDMIInAVLatency(const int32_t audioDelay, const int32_t videoDelay);
            void OnHDMIInVRRStatus(const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort port, const Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInVRRType vrrType);

            //  IPlugin methods
            // -------------------------------------------------------------------------------------------------------
            virtual const string Initialize(PluginHost::IShell *service) override;
            virtual void Deinitialize(PluginHost::IShell *service) override;
            virtual string Information() const override;

        private:
            PluginHost::IShell* _service{};
            uint32_t _connectionId{};
            Exchange::IPowerManager* _powerManager;
            // Removed: Exchange::IDeviceSettingsManagerFPD* _fpdManager; // Not needed, use IDeviceSettingsManager only
            Exchange::IDeviceSettingsManager* _deviceSettingsManager;
            //PowerManagerInterfaceRef _powerManager{};
            Core::Sink<PowerManagerNotification> _pwrMgrNotification;
            Core::Sink<HDMIInNotification> _hdmiInNotification;

	    void Deactivated(RPC::IRemoteConnection *connection);
        };

    } // namespace Plugin
} // namespace WPEFramework

#endif // USERPLUGIN_USERPLUGIN_H
