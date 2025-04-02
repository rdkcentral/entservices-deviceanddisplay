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
                ,userSettings(nullptr)
                , _service(nullptr)
                , _notification(this)
                , _migrationDone(false)
                ,_lastUILanguage("")
        {
            LOGINFO("ctor");
            UserPreferences::_instance = this;
            Register("getUILanguage", &UserPreferences::getUILanguage, this);
            Register("setUILanguage", &UserPreferences::setUILanguage, this);
        }

        UserPreferences::~UserPreferences()
        {
            //LOGINFO("dtor");
            if (_service != nullptr) {
                _service->Release();
                _service = nullptr;
            }
        }

        const string UserPreferences::Initialize(PluginHost::IShell* shell) {
            LOGINFO("Initializing UserPreferences plugin");
            _service = shell;
            _service->AddRef();
            
            // Register for onPresentationLanguageChanged notification
            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            LOGINFO("usersetting %p",userSettings);
            if (userSettings == nullptr) {
                LOGERR("Failed to get UserSettings interface");
                return "Failed to initialize: UserSettings interface not available";
            }
        
            userSettings->Register(&_notification);
        
            // Check migration state
            Exchange::IUserSettingsInspector* userSettingsInspector = _service->QueryInterfaceByCallsign<Exchange::IUserSettingsInspector>("org.rdk.UserSettings");
            if (userSettingsInspector == nullptr) {
                LOGERR("Failed to get UserSettingsInspector interface");
                userSettings->Unregister(&_notification);
                userSettings->Release();
                return "Failed to initialize: UserSettingsInspector interface not available";
            }
        
            bool requiresMigration = false;
            uint32_t status = userSettingsInspector->GetMigrationState(Exchange::IUserSettingsInspector::SettingsKey::PRESENTATION_LANGUAGE, requiresMigration);
            if (status != Core::ERROR_NONE) {
                LOGERR("Failed to get migration state: %u", status);
                userSettingsInspector->Release();
                userSettings->Release();
                return "Failed to initialize: Could not determine migration state";
            }
        
            g_autoptr(GKeyFile) file = g_key_file_new();
            g_autoptr(GError) error = nullptr;
        
            if (requiresMigration) {
                LOGINFO("Migration is required for presentation language");
        
                if (g_key_file_load_from_file(file, SETTINGS_FILE_NAME, G_KEY_FILE_NONE, &error)) {
                    // File exists and was successfully loaded
                    g_autofree gchar *val = g_key_file_get_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, &error);
                    if (val != nullptr) {
                        string uiLanguage = val;
                        // Convert UI language to presentation language format
                        size_t sep = uiLanguage.find('_');
                        if (sep == 2 && uiLanguage.length() == 5) {
                            string presentationLanguage = uiLanguage.substr(sep + 1) + "-" + uiLanguage.substr(0, sep);
                            status = userSettings->SetPresentationLanguage(presentationLanguage);
                            if (status != Core::ERROR_NONE) {
                                LOGERR("Failed to set presentation language: %u", status);
                            }
                        } else {
                            LOGERR("Invalid UI language format in file: %s", uiLanguage.c_str());
                        }
                    } else {
                        LOGERR("Failed to read UI language from file: %s", error->message);
                    }
                } else if (error != nullptr && error->code == G_FILE_ERROR_NOENT) {
                    string presentationLanguage;
                    status = userSettings->GetPresentationLanguage(presentationLanguage);
                    if (status == Core::ERROR_NONE) {
                        // Convert presentation language to UI language format
                        size_t sep = presentationLanguage.find('-');
                        if (sep == 2 && presentationLanguage.length() == 5) {
                            string uiLanguage = presentationLanguage.substr(sep + 1) + "_" + presentationLanguage.substr(0, sep);
                            g_key_file_set_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, (gchar*)uiLanguage.c_str());
                            if (!g_key_file_save_to_file(file, SETTINGS_FILE_NAME, &error)) {
                                LOGERR("Failed to save file: %s", error->message);
                            }
                        } else {
                            LOGERR("Invalid presentation language: %s", presentationLanguage.c_str());
                        }
                    } else {
                        LOGERR("Failed to get presentation language: %u", status);
                    }
                } else {
                    LOGERR("Failed to load file: %s", error->message);
                }
            } else {
                LOGINFO("No migration required for presentation language");
        
                string presentationLanguage;
                status = userSettings->GetPresentationLanguage(presentationLanguage);
                if (status == Core::ERROR_NONE) {
                    // Convert presentation language to UI language format
                    size_t sep = presentationLanguage.find('-');
                    if (sep == 2 && presentationLanguage.length() == 5) {
                        string uiLanguage = presentationLanguage.substr(sep + 1) + "_" + presentationLanguage.substr(0, sep);
                        g_key_file_set_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, (gchar*)uiLanguage.c_str());
                        if (!g_key_file_save_to_file(file, SETTINGS_FILE_NAME, &error)) {
                            LOGERR("Failed to save file: %s", error->message);
                        }
                    } else {
                        LOGERR("Invalid presentation language: %s", presentationLanguage.c_str());
                    }
                } else {
                    LOGERR("Failed to get presentation language: %u", status);
                }
            }
            _migrationDone = true;
        
            userSettingsInspector->Release();
            userSettings->Release();
            return {};
        }

        void UserPreferences::Deinitialize(PluginHost::IShell* /* service */)
        {
            LOGINFO("Deinitialize");
            if (userSettings != nullptr)
            {
                userSettings->Unregister(&_notification);
                userSettings->Release();
                userSettings = nullptr;
            }
            if (_service != nullptr)
            {
                _service->Release();
                _service = nullptr;
            }
            UserPreferences::_instance = nullptr;
        }

        void UserPreferences::Notification::OnPresentationLanguageChanged(const string& language) {
             _parent->OnPresentationLanguageChanged(language);
        }

        void UserPreferences::OnPresentationLanguageChanged(const string& language) {
            LOGINFO("Presentation language changed to: %s", language.c_str());
            // Convert presentation language to UI language format
            size_t sep = language.find('-');
            if (sep == 2 && language.length() == 5) {
                string uiLanguage = language.substr(sep + 1) + "_" + language.substr(0, sep);
                g_autoptr(GKeyFile) file = g_key_file_new();
                g_key_file_set_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, (gchar *)uiLanguage.c_str());
                g_autoptr(GError) error = nullptr;
                if (!g_key_file_save_to_file(file, SETTINGS_FILE_NAME, &error)) {
                    LOGERR("Error saving file '%s': %s", SETTINGS_FILE_NAME, error->message);
                }
            } else {
                LOGERR("Invalid presentation language format: %s", language.c_str());
            }
        }

        void UserPreferences::Notification::OnAudioDescriptionChanged(const bool enabled)  {
            
        }
        void UserPreferences::Notification::OnPreferredAudioLanguagesChanged(const string& preferredLanguages)  {

        }
        void UserPreferences::Notification::OnCaptionsChanged(const bool enabled)  {

        }
        void UserPreferences::Notification::OnPreferredCaptionsLanguagesChanged(const string& preferredLanguages)  {

        }
       
        void UserPreferences::Notification::OnPreferredClosedCaptionServiceChanged(const string& service)  {

        }
        
        void UserPreferences::Notification::OnPinControlChanged(const bool pinControl)  {

        }
       
        void UserPreferences::Notification::OnViewingRestrictionsChanged(const string& viewingRestrictions)  {

        }
       
        void UserPreferences::Notification::OnViewingRestrictionsWindowChanged(const string& viewingRestrictionsWindow)  {

        }
      
        void UserPreferences::Notification::OnLiveWatershedChanged(const bool liveWatershed)  {

        }
       
        void UserPreferences::Notification::OnPlaybackWatershedChanged(const bool playbackWatershed)  {

        }
       
        void UserPreferences::Notification::OnBlockNotRatedContentChanged(const bool blockNotRatedContent)  {

        }
      
        void UserPreferences::Notification::OnPinOnPurchaseChanged(const bool pinOnPurchase)  {

        }
        
        void UserPreferences::Notification::OnHighContrastChanged(const bool enabled)  {

        }
        void UserPreferences::Notification::OnVoiceGuidanceChanged(const bool enabled)  {

        }
        void UserPreferences::Notification::OnVoiceGuidanceRateChanged(const double rate)  {

        }
        
        void UserPreferences::Notification::OnVoiceGuidanceHintsChanged(const bool hints)  {

        }
        void UserPreferences::Notification::AddRef() const {
            
        }

        uint32_t UserPreferences::Notification::Release() const {
           return 0;
        }

        //Begin methods
        uint32_t UserPreferences::getUILanguage(const JsonObject& parameters, JsonObject& response) {
            LOGINFOMETHOD();
	    if (!_migrationDone) {
                LOGERR("Migration not complete, cannot get UI language");
                returnResponse(false);
            }
            string language;

            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            if (userSettings != nullptr) {
                string presentationLanguage;
                uint32_t status = userSettings->GetPresentationLanguage(presentationLanguage);
                if (status == Core::ERROR_NONE) {
                    // Convert presentation language to UI language format
                    size_t sep = presentationLanguage.find('-');
                    if (sep == 2 && presentationLanguage.length() == 5) {
                        language = presentationLanguage.substr(sep + 1) + "_" + presentationLanguage.substr(0, sep);
                        // Optimization: Update file only if language has changed
                        if (language != _lastUILanguage) {
                            g_autoptr(GKeyFile) file = g_key_file_new();
                            g_key_file_set_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, (gchar*)language.c_str());
                            g_autoptr(GError) error = nullptr;
                            if (g_key_file_save_to_file(file, SETTINGS_FILE_NAME, &error)) {
                                _lastUILanguage = language; // Update last known UI language
                            } else {
                                LOGERR("Error saving file '%s': %s", SETTINGS_FILE_NAME, error->message);
                            }
                        }
                    } else {
                        LOGERR("Invalid presentation language format: %s", presentationLanguage.c_str());
                    }
                } else {
                    LOGERR("Failed to get presentation language: %u", status);
                }
                userSettings->Release();
            } else {
                LOGERR("Failed to get UserSettings interface");
                returnResponse(false);
            }

            response[SETTINGS_FILE_KEY] = language;
            returnResponse(true);
        }

          

        uint32_t UserPreferences::setUILanguage(const JsonObject& parameters, JsonObject& response) {
        LOGINFOMETHOD();
	    if (!_migrationDone) {
                LOGERR("Migration not complete, cannot set UI language");
                returnResponse(false);
            }
            returnIfStringParamNotFound(parameters, SETTINGS_FILE_KEY);
            string uiLanguage = parameters[SETTINGS_FILE_KEY].String();

            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            if (userSettings != nullptr) {
                // Convert UI language to presentation language format
                size_t sep = uiLanguage.find('_');
                if (sep == 2 && uiLanguage.length() == 5) {
                    string presentationLanguage = uiLanguage.substr(sep + 1) + "-" + uiLanguage.substr(0, sep);
                    uint32_t status = userSettings->SetPresentationLanguage(presentationLanguage);
                    if (status != Core::ERROR_NONE) {
                        LOGERR("Failed to set presentation language in UserSettings");
                        userSettings->Release();
                        returnResponse(false);
                    }
                } else {
                    LOGERR("Invalid UI language format: %s", uiLanguage.c_str());
                    userSettings->Release();
                    returnResponse(false);
                }
                userSettings->Release();
            } else {
                LOGERR("Failed to get UserSettings interface");
                returnResponse(false);
            }

            returnResponse(true);
        }
        //End methods

        //Begin events
        //End events

    } // namespace Plugin
} // namespace WPEFramework
