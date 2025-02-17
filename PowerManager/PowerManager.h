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

#pragma once

#include "Module.h"
#include <interfaces/json/JsonData_PowerManager.h>
#include <interfaces/json/JPowerManager.h>
#include <interfaces/IPowerManager.h>
#include "UtilsLogging.h"
#include "tracing/Logging.h"
#include <mutex>

using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
using ThermalTemperature = WPEFramework::Exchange::IPowerManager::ThermalTemperature;

namespace WPEFramework {
namespace Plugin {

    class PowerManager: public PluginHost::IPlugin, public PluginHost::JSONRPC
    {
     private:
        class Notification : public RPC::IRemoteConnection::INotification,
                             public Exchange::IPowerManager::INotification
        {
            private:
                Notification() = delete;
                Notification(const Notification&) = delete;
                Notification& operator=(const Notification&) = delete;

            public:
            explicit Notification(PowerManager* parent)
                : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }

                virtual ~Notification()
                {
                }

                BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(Exchange::IPowerManager::INotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                END_INTERFACE_MAP

                void Activated(RPC::IRemoteConnection*) override
                {
                }

                void Deactivated(RPC::IRemoteConnection *connection) override
                {
                   _parent.Deactivated(connection);
                }

                void OnPowerModeChanged(const PowerState &currentState, const PowerState &newState) override
                {
                    LOGINFO("onPowerModeChanged: currentState %u, newState %u", currentState, newState);
                    Exchange::JPowerManager::Event::OnPowerModeChanged(_parent, currentState, newState);
                }

                void OnPowerModePreChange(const PowerState &currentState, const PowerState &newState) override
                {
                    LOGINFO("onPowerModePreChange: currentState %u, newState %u", currentState, newState);
                    Exchange::JPowerManager::Event::OnPowerModePreChange(_parent, currentState, newState);
                }

                void OnDeepSleepTimeout(const int &wakeupTimeout) override
                {
                    LOGINFO("onDeepSleepTimeout: wakeupTimeout %d", wakeupTimeout);
                    Exchange::JPowerManager::Event::OnDeepSleepTimeout(_parent, wakeupTimeout);
                }

                void OnNetworkStandbyModeChanged(const bool &enabled) override
                {
                    LOGINFO("onNetworkStandbyModeChanged: enabled %d", enabled);
                    Exchange::JPowerManager::Event::OnNetworkStandbyModeChanged(_parent, enabled);
                }

                void OnThermalModeChanged(const ThermalTemperature &currentThermalLevel, const ThermalTemperature &newThermalLevel, const float &currentTemperature) override
                {
                    LOGINFO("onThermalModeChanged: currentThermalLevel %d, newThermalLevel %d, currentTemperature %f", currentThermalLevel, newThermalLevel, currentTemperature);
                    Exchange::JPowerManager::Event::OnThermalModeChanged(_parent, currentThermalLevel, newThermalLevel, currentTemperature );
                }

                void OnRebootBegin(const string &rebootReasonCustom, const string &rebootReasonOther, const string &rebootRequestor) override
                {
                    LOGINFO("onRebootBegin: rebootReasonCustom %s, rebootReasonOther %s, rebootRequestor %s", rebootReasonCustom.c_str(), rebootReasonOther.c_str(), rebootRequestor.c_str());
                    Exchange::JPowerManager::Event::OnRebootBegin(_parent, rebootReasonCustom, rebootReasonOther, rebootRequestor);
                }

            private:
                PowerManager& _parent;
        };

        public:
            // We do not allow this plugin to be copied !!
            PowerManager(const PowerManager&) = delete;
            PowerManager& operator=(const PowerManager&) = delete;

            PowerManager();
            virtual ~PowerManager();

            BEGIN_INTERFACE_MAP(PowerManager)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IPowerManager, _powerManager)
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
            Exchange::IPowerManager* _powerManager{};
            Core::Sink<Notification> _powermanagersNotification;
    };

} // namespace Plugin
} // namespace WPEFramework
