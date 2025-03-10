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

#include "UserPreferences.h"
#include "UtilsJsonRpc.h"

#include <glib.h>
#include <glib/gstdio.h>

#define SETTINGS_FILE_NAME              "/opt/user_preferences.conf"
#define SETTINGS_FILE_KEY               "ui_language"
#define SETTINGS_FILE_GROUP              "General"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

using namespace std;

namespace WPEFramework {

    namespace {

        static Plugin::Metadata<Plugin::UserPreferences> metadata(
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

        SERVICE_REGISTRATION(UserPreferences, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

        UserPreferences* UserPreferences::_instance = nullptr;

        UserPreferences::UserPreferences()
                : PluginHost::JSONRPC()
                , _service(nullptr)
                , _userSettings(nullptr)
        {
            LOGINFO("ctor");
            UserPreferences::_instance = this;
            Register("getUILanguage", &UserPreferences::getUILanguage, this);
            Register("setUILanguage", &UserPreferences::setUILanguage, this);
        }

        UserPreferences::~UserPreferences()
        {
            if (_userSettings != nullptr) {
                _userSettings->Release();
                _userSettings = nullptr;
            }
            if (_service != nullptr) {
                _service->Release();
                _service = nullptr;
            }
            //LOGINFO("dtor");
        }

        const string UserPreferences::Initialize(PluginHost::IShell* shell)
        {
            LOGINFO("Initializing UserPreferences plugin");

            if (shell != nullptr) {
                _service = shell;
                _service->AddRef();

                // Attempt to connect to UserSettings plugin
                _userSettings = _service->QueryInterfaceByCallsign<WPEFramework::Exchange::IUserSettings>("org.rdk.UserSettings");
                if (_userSettings != nullptr) {
                    LOGINFO("Successfully connected to UserSettings plugin");
                } else {
                    LOGERR("Failed to connect to UserSettings plugin");
                }
            } else {
                LOGERR("Shell is null");
            }

            return {}; // Return empty string to indicate success
        }

        void UserPreferences::Deinitialize(PluginHost::IShell* /* service */)
        {
            LOGINFO("Deinitialize");
            UserPreferences::_instance = nullptr;
        }

        //Begin methods
        uint32_t UserPreferences::getUILanguage(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();
            string language;
            if (_userSettings != nullptr) {
                uint32_t status = _userSettings->GetPresentationLanguage(language);
                if (status != Core::ERROR_NONE) {
                    LOGERR("Failed to get presentation language from UserSettings");
                    returnResponse(false);
                }
            }else{
                g_autoptr(GKeyFile) file = g_key_file_new();
                g_autoptr(GError) error = nullptr;
                if (!g_key_file_load_from_file(file, SETTINGS_FILE_NAME, G_KEY_FILE_NONE, &error)) {
                    LOGERR("Unable to load from file '%s': %s", SETTINGS_FILE_NAME, error->message);
                    returnResponse(false);
                }

                g_autofree gchar *val = g_key_file_get_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, &error);
                if (val == nullptr) {
                    LOGERR("Unable to get key '%s' for group '%s' from file '%s': %s"
                            , SETTINGS_FILE_KEY, SETTINGS_FILE_GROUP, SETTINGS_FILE_NAME, error->message);
                    returnResponse(false);
                }

                language = val;
            }

            response[SETTINGS_FILE_KEY] = language;
            returnResponse(true);
        }

        uint32_t UserPreferences::setUILanguage(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();
            returnIfStringParamNotFound(parameters, SETTINGS_FILE_KEY);
            string language = parameters[SETTINGS_FILE_KEY].String();

            if (_userSettings != nullptr) {
                uint32_t status = _userSettings->SetPresentationLanguage(language);
                if (status != Core::ERROR_NONE) {
                    LOGERR("Failed to set presentation language in UserSettings");
                    returnResponse(false);
                }
            }

            g_autoptr(GKeyFile) file = g_key_file_new();
            g_key_file_set_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, (gchar *)language.c_str());

            g_autoptr(GError) error = nullptr;
            if (!g_key_file_save_to_file(file, SETTINGS_FILE_NAME, &error)) {
                LOGERR("Error saving file '%s': %s", SETTINGS_FILE_NAME, error->message);
                returnResponse(false);
            }

            returnResponse(true);
        }
        //End methods

        //Begin events
        //End events

    } // namespace Plugin
} // namespace WPEFramework
