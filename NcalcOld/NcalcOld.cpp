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

#include "NcalcOld.h"

using namespace std;
using namespace WPEFramework;

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {

    namespace {
        static Plugin::Metadata<Plugin::NcalcOld> metadata(
            // Version (Major, Minor, Patch)
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    namespace Plugin {

        SERVICE_REGISTRATION(NcalcOld, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

        NcalcOld::NcalcOld()
            : PluginHost::JSONRPC()
            , m_service(nullptr)
        {
            LOGINFO("NcalcOld Constructor");
        }

        NcalcOld::~NcalcOld()
        {
            LOGINFO("NcalcOld Destructor");
        }

        const string NcalcOld::Initialize(PluginHost::IShell* service)
        {
            ASSERT(service != nullptr);
            ASSERT(m_service == nullptr);

            m_service = service;
            m_service->AddRef();

            // Register JSONRPC methods
            Register<JsonObject, JsonObject>(_T("addition"), &NcalcOld::addition, this);
            Register<JsonObject, JsonObject>(_T("subtraction"), &NcalcOld::subtraction, this);
            Register<JsonObject, JsonObject>(_T("multiply"), &NcalcOld::multiply, this);
            Register<JsonObject, JsonObject>(_T("division"), &NcalcOld::division, this);

            LOGINFO("NcalcOld plugin initialized successfully");
            return string();
        }

        void NcalcOld::Deinitialize(PluginHost::IShell* service)
        {
            ASSERT(m_service == service);

            // Unregister JSONRPC methods
            Unregister(_T("addition"));
            Unregister(_T("subtraction"));
            Unregister(_T("multiply"));
            Unregister(_T("division"));

            m_service->Release();
            m_service = nullptr;

            LOGINFO("NcalcOld plugin deinitialized");
        }

        string NcalcOld::Information() const
        {
            return string();
        }

        // JSONRPC method implementations
        uint32_t NcalcOld::addition(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFO("addition called");

            if (!parameters.HasLabel("a") || !parameters.HasLabel("b")) {
                LOGERR("Missing parameters 'a' or 'b'");
                response["error"] = "Missing parameters 'a' or 'b'";
                return Core::ERROR_GENERAL;
            }

            int a = parameters["a"].Number();
            int b = parameters["b"].Number();
            int result = a + b;

            LOGINFO("Addition: %d + %d = %d", a, b, result);

            response["result"] = result;
            response["success"] = true;

            return Core::ERROR_NONE;
        }

        uint32_t NcalcOld::subtraction(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFO("subtraction called");

            if (!parameters.HasLabel("a") || !parameters.HasLabel("b")) {
                LOGERR("Missing parameters 'a' or 'b'");
                response["error"] = "Missing parameters 'a' or 'b'";
                return Core::ERROR_GENERAL;
            }

            int a = parameters["a"].Number();
            int b = parameters["b"].Number();
            int result = a - b;

            LOGINFO("Subtraction: %d - %d = %d", a, b, result);

            response["result"] = result;
            response["success"] = true;

            return Core::ERROR_NONE;
        }

        uint32_t NcalcOld::multiply(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFO("multiply called");

            if (!parameters.HasLabel("a") || !parameters.HasLabel("b")) {
                LOGERR("Missing parameters 'a' or 'b'");
                response["error"] = "Missing parameters 'a' or 'b'";
                return Core::ERROR_GENERAL;
            }

            int a = parameters["a"].Number();
            int b = parameters["b"].Number();
            int result = a * b;

            LOGINFO("Multiply: %d * %d = %d", a, b, result);

            response["result"] = result;
            response["success"] = true;

            return Core::ERROR_NONE;
        }

        uint32_t NcalcOld::division(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFO("division called");

            if (!parameters.HasLabel("a") || !parameters.HasLabel("b")) {
                LOGERR("Missing parameters 'a' or 'b'");
                response["error"] = "Missing parameters 'a' or 'b'";
                return Core::ERROR_GENERAL;
            }

            int a = parameters["a"].Number();
            int b = parameters["b"].Number();

            if (b == 0) {
                LOGERR("Division by zero");
                response["error"] = "Division by zero";
                response["success"] = false;
                return Core::ERROR_GENERAL;
            }

            int result = a / b;

            LOGINFO("Division: %d / %d = %d", a, b, result);

            response["result"] = result;
            response["success"] = true;

            return Core::ERROR_NONE;
        }

    } // namespace Plugin
} // namespace WPEFramework
