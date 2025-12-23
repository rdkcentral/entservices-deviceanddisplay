/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2025 RDK Management
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

#ifndef NCALCOLD_H
#define NCALCOLD_H

#include "Module.h"
#include "UtilsLogging.h"
#include "tracing/Logging.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {
    namespace Plugin {

        class NcalcOld : public PluginHost::IPlugin, public PluginHost::JSONRPC {
        private:
            // We do not allow this plugin to be copied
            NcalcOld(const NcalcOld&) = delete;
            NcalcOld& operator=(const NcalcOld&) = delete;

        public:
            NcalcOld();
            virtual ~NcalcOld();

            BEGIN_INTERFACE_MAP(NcalcOld)
                INTERFACE_ENTRY(PluginHost::IPlugin)
                INTERFACE_ENTRY(PluginHost::IDispatcher)
            END_INTERFACE_MAP

            // IPlugin methods
            virtual const string Initialize(PluginHost::IShell* service) override;
            virtual void Deinitialize(PluginHost::IShell* service) override;
            virtual string Information() const override;

        private:
            // JSONRPC methods
            uint32_t addition(const JsonObject& parameters, JsonObject& response);
            uint32_t subtraction(const JsonObject& parameters, JsonObject& response);
            uint32_t multiply(const JsonObject& parameters, JsonObject& response);
            uint32_t division(const JsonObject& parameters, JsonObject& response);

        private:
            PluginHost::IShell* m_service;
        };

    } // namespace Plugin
} // namespace WPEFramework

#endif // NCALCOLD_H
