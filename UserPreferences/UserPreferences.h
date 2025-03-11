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
#include <interfaces/IStore2.h>
#include <interfaces/IUserSettings.h>

#include "Module.h"

namespace WPEFramework {
    namespace Plugin {

        class UserPreferences : public PluginHost::IPlugin, public PluginHost::JSONRPC {
        private:

            UserPreferences(const UserPreferences&) = delete;
            UserPreferences& operator=(const UserPreferences&) = delete;

            class Notification : public Exchange::IUserSettings::INotification {
            public:
                explicit Notification(UserPreferences* parent) : _parent(parent) {}
                ~Notification() override = default;

                void onPresentationLanguageChanged(const string& language) override;

            private:
                UserPreferences* _parent;
            };

            //Begin methods
            uint32_t getUILanguage(const JsonObject& parameters, JsonObject& response);
            uint32_t setUILanguage(const JsonObject& parameters, JsonObject& response);
            //End methods

            //Begin events
            //End events

        public:
            UserPreferences();
            virtual ~UserPreferences();
            virtual const string Initialize(PluginHost::IShell* shell) override ;
            virtual void Deinitialize(PluginHost::IShell* service) override;
            virtual string Information() const override { return {}; }

            BEGIN_INTERFACE_MAP(UserPreferences)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            END_INTERFACE_MAP

        private:

            void OnPresentationLanguageChanged(const string& language);

            Exchange::IUserSettings* _userSettings;
            PluginHost::IShell* _service;
            Notification _notification;

        public:
            static UserPreferences* _instance;

        };
    } // namespace Plugin
} // namespace WPEFramework
