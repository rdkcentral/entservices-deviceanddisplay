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
	    Core::hresult GetCurrentResolutionValue(std::string& resolution) const;
            void OnPowerModeChanged(const PowerState currentState, const PowerState newState);
            void onPowerStateChanged(string currentPowerState, string powerState);

            //  IPlugin methods
            // -------------------------------------------------------------------------------------------------------
            virtual const string Initialize(PluginHost::IShell *service) override;
            virtual void Deinitialize(PluginHost::IShell *service) override;
            virtual string Information() const override;
            // -------------------------------------------------------------------------------------------------------

        private:
            PluginHost::IShell* _service{};
            uint32_t _connectionId{};
            Exchange::IPowerManager* _powerManager;
            //PowerManagerInterfaceRef _powerManager{};
            Core::Sink<PowerManagerNotification> _pwrMgrNotification;

	    void Deactivated(RPC::IRemoteConnection *connection);
        };

    } // namespace Plugin
} // namespace WPEFramework

#endif // USERPLUGIN_USERPLUGIN_H
