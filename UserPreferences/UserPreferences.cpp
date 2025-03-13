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

        static const unordered_map<string, string> presentationToUILang = {
            {"en-US", "US_en"}, {"es-US", "US_es"}, {"en-CA", "CA_en"}, {"fr-CA", "CA_fr"},
            {"en-GB", "GB_en"}, {"it-IT", "IT_it"}, {"de-DE", "DE_de"}, {"en-AU", "AU_en"}
        };

        static const unordered_map<string, string> uiToPresentationLang = {
            {"US_en", "en-US"}, {"US_es", "es-US"}, {"CA_en", "en-CA"}, {"CA_fr", "fr-CA"},
            {"GB_en", "en-GB"}, {"IT_it", "it-IT"}, {"DE_de", "de-DE"}, {"AU_en", "en-AU"}
        };

        UserPreferences::UserPreferences()
                : PluginHost::JSONRPC()
				, _service(nullptr)
                , _notification(this)
        {
            LOGINFO("ctor");
            UserPreferences::_instance = this;
            Register("getUILanguage", &UserPreferences::getUILanguage, this);
            Register("setUILanguage", &UserPreferences::setUILanguage, this);
        }

        UserPreferences::~UserPreferences()
        {
            
            if (_service != nullptr) {
                _service->Release();
                _service = nullptr;
            }
            //LOGINFO("dtor");
        }

        const string UserPreferences::Initialize(PluginHost::IShell* shell) {
            LOGINFO("Initializing UserPreferences plugin");
            _service = shell;
            _service->AddRef();
            return {};
        }

        void UserPreferences::Deinitialize(PluginHost::IShell* /* service */)
        {
            LOGINFO("Deinitialize");
            UserPreferences::_instance = nullptr;
        }

        void UserPreferences::Notification::OnPresentationLanguageChanged(const string& language) {
            _parent->OnPresentationLanguageChanged(language);
        }

        void UserPreferences::OnPresentationLanguageChanged(const string& language)  {

            LOGINFO("Presentation language changed to: %s", language.c_str());

            // Convert presentation language to UI language
            auto it = presentationToUILang.find(language);
            if (it != presentationToUILang.end()) {
                string uiLanguage = it->second;
                // Update the file with the new value
                g_autoptr(GKeyFile) file = g_key_file_new();
                g_key_file_set_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, (gchar *)uiLanguage.c_str());

                g_autoptr(GError) error = nullptr;
                if (!g_key_file_save_to_file(file, SETTINGS_FILE_NAME, &error)) {
                    LOGERR("Error saving file '%s': %s", SETTINGS_FILE_NAME, error->message);
                }
            } else {
                LOGERR("Failed to convert presentation language to UI language: %s", language.c_str());
            }
        }
        void OnAudioDescriptionChanged(const bool enabled)  {
            
        }
        void OnPreferredAudioLanguagesChanged(const string& preferredLanguages)  {

        }
       void OnCaptionsChanged(const bool enabled)  {

        }
        void OnPreferredCaptionsLanguagesChanged(const string& preferredLanguages)  {

        }
       
        void OnPreferredClosedCaptionServiceChanged(const string& service)  {

        }
        
        void OnPinControlChanged(const bool pinControl)  {

        }
       
        void OnViewingRestrictionsChanged(const string& viewingRestrictions)  {

        }
       
        void OnViewingRestrictionsWindowChanged(const string& viewingRestrictionsWindow)  {

        }
      
        void OnLiveWatershedChanged(const bool liveWatershed)  {

        }
       
        void OnPlaybackWatershedChanged(const bool playbackWatershed)  {

        }
       
        void OnBlockNotRatedContentChanged(const bool blockNotRatedContent)  {

        }
      
        void OnPinOnPurchaseChanged(const bool pinOnPurchase)  {

        }
        
        void OnHighContrastChanged(const bool enabled)  {

        }
        void OnVoiceGuidanceChanged(const bool enabled)  {

        }
        void OnVoiceGuidanceRateChanged(const double rate)  {

        }
        
        void OnVoiceGuidanceHintsChanged(const bool hints)  {

        }
        
        //Begin methods
        uint32_t UserPreferences::getUILanguage(const JsonObject& parameters, JsonObject& response) {
            LOGINFOMETHOD();
            string language;

            // Acquire UserSettings and UserSettingsInspector references dynamically
            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            Exchange::IUserSettingsInspector* userSettingsInspector = _service->QueryInterfaceByCallsign<Exchange::IUserSettingsInspector>("org.rdk.UserSettings");

            if (userSettings != nullptr && userSettingsInspector != nullptr) {
                // Check if migration is required
                bool requiresMigration = false;
                uint32_t status = userSettingsInspector->GetMigrationState(Exchange::IUserSettingsInspector::SettingsKey::PRESENTATION_LANGUAGE, requiresMigration);
                if (status == Core::ERROR_NONE) {
                    if (requiresMigration) {
                        // Migrate value from file to UserSettings
                        g_autoptr(GKeyFile) file = g_key_file_new();
                        g_autoptr(GError) error = nullptr;
                        if (g_key_file_load_from_file(file, SETTINGS_FILE_NAME, G_KEY_FILE_NONE, &error)) {
                            g_autofree gchar *val = g_key_file_get_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, &error);
                            if (val != nullptr) {
                                string uiLanguage = val;
                                auto it = uiToPresentationLang.find(uiLanguage);
                                if (it != uiToPresentationLang.end()) {
                                    string presentationLanguage = it->second;
                                    status = userSettings->SetPresentationLanguage(presentationLanguage);
                                    if (status != Core::ERROR_NONE) {
                                        LOGERR("Failed to set presentation language in UserSettings");
                                        userSettings->Release();
                                        userSettingsInspector->Release();
                                        returnResponse(false);
                                    }
                                } else {
                                    LOGERR("Failed to convert UI language to presentation language: %s", uiLanguage.c_str());
                                    userSettings->Release();
                                    userSettingsInspector->Release();
                                    returnResponse(false);
                                }
                            }
                        }
                    }

                    // Always read the value using GetPresentationLanguage
                    string presentationLanguage;
                    status = userSettings->GetPresentationLanguage(presentationLanguage);
                    if (status == Core::ERROR_NONE) {
                        auto it = presentationToUILang.find(presentationLanguage);
                        if (it != presentationToUILang.end()) {
                            language = it->second;
                            // Refresh the file with the latest value
                            g_autoptr(GKeyFile) file = g_key_file_new();
                            g_key_file_set_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, (gchar *)language.c_str());

                            g_autoptr(GError) error = nullptr;
                            if (!g_key_file_save_to_file(file, SETTINGS_FILE_NAME, &error)) {
                                LOGERR("Error saving file '%s': %s", SETTINGS_FILE_NAME, error->message);
                            } else {
                                LOGINFO("Refreshed file with language value: %s", language.c_str());
                            }
                        } else {
                            LOGERR("Failed to convert presentation language to UI language: %s", presentationLanguage.c_str());
                            userSettings->Release();
                            userSettingsInspector->Release();
                            returnResponse(false);
                        }
                    } else {
                        LOGERR("Failed to get presentation language from UserSettings");
                        userSettings->Release();
                        userSettingsInspector->Release();
                        returnResponse(false);
                    }
                } else {
                    LOGERR("Failed to get migration state");
                    userSettings->Release();
                    userSettingsInspector->Release();
                    returnResponse(false);
                }

                userSettings->Release();
                userSettingsInspector->Release();
            } else {
                LOGERR("Failed to connect to UserSettings or UserSettingsInspector plugin");
                if (userSettings != nullptr) userSettings->Release();
                if (userSettingsInspector != nullptr) userSettingsInspector->Release();
                returnResponse(false);
            }

            response[SETTINGS_FILE_KEY] = language;
            returnResponse(true);
        }

        uint32_t UserPreferences::setUILanguage(const JsonObject& parameters, JsonObject& response) {
            LOGINFOMETHOD();
            returnIfStringParamNotFound(parameters, SETTINGS_FILE_KEY);
            string uiLanguage = parameters[SETTINGS_FILE_KEY].String();

            // Acquire UserSettings and UserSettingsInspector references dynamically
            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            Exchange::IUserSettingsInspector* userSettingsInspector = _service->QueryInterfaceByCallsign<Exchange::IUserSettingsInspector>("org.rdk.UserSettings");

            if (userSettings != nullptr && userSettingsInspector != nullptr) {
                // Check if migration is required
                bool requiresMigration = false;
                uint32_t status = userSettingsInspector->GetMigrationState(Exchange::IUserSettingsInspector::SettingsKey::PRESENTATION_LANGUAGE, requiresMigration);
                if (status == Core::ERROR_NONE) {
                    if (requiresMigration) {
                        // Migrate value from file to UserSettings
                        g_autoptr(GKeyFile) file = g_key_file_new();
                        g_autoptr(GError) error = nullptr;
                        if (g_key_file_load_from_file(file, SETTINGS_FILE_NAME, G_KEY_FILE_NONE, &error)) {
                            g_autofree gchar *val = g_key_file_get_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, &error);
                            if (val != nullptr) {
                                string uiLanguage = val;
                                auto it = uiToPresentationLang.find(uiLanguage);
                                if (it != uiToPresentationLang.end()) {
                                    string presentationLanguage = it->second;
                                    status = userSettings->SetPresentationLanguage(presentationLanguage);
                                    if (status != Core::ERROR_NONE) {
                                        LOGERR("Failed to set presentation language in UserSettings");
                                        userSettings->Release();
                                        userSettingsInspector->Release();
                                        returnResponse(false);
                                    }
                                } else {
                                    LOGERR("Failed to convert UI language to presentation language: %s", uiLanguage.c_str());
                                    userSettings->Release();
                                    userSettingsInspector->Release();
                                    returnResponse(false);
                                }
                            }
                        }
                    }

                    // Always set the value using SetPresentationLanguage
                    auto it = uiToPresentationLang.find(uiLanguage);
                    if (it != uiToPresentationLang.end()) {
                        string presentationLanguage = it->second;
                        status = userSettings->SetPresentationLanguage(presentationLanguage);
                        if (status != Core::ERROR_NONE) {
                            LOGERR("Failed to set presentation language in UserSettings");
                            userSettings->Release();
                            userSettingsInspector->Release();
                            returnResponse(false);
                        }

                        // Update the file with the new value
                        g_autoptr(GKeyFile) file = g_key_file_new();
                        g_key_file_set_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, (gchar *)uiLanguage.c_str());

                        g_autoptr(GError) error = nullptr;
                        if (!g_key_file_save_to_file(file, SETTINGS_FILE_NAME, &error)) {
                            LOGERR("Error saving file '%s': %s", SETTINGS_FILE_NAME, error->message);
                            userSettings->Release();
                            userSettingsInspector->Release();
                            returnResponse(false);
                        }
                    } else {
                        LOGERR("Failed to convert UI language to presentation language: %s", uiLanguage.c_str());
                        userSettings->Release();
                        userSettingsInspector->Release();
                        returnResponse(false);
                    }
                } else {
                    LOGERR("Failed to get migration state");
                    userSettings->Release();
                    userSettingsInspector->Release();
                    returnResponse(false);
                }

                userSettings->Release();
                userSettingsInspector->Release();
            } else {
                LOGERR("Failed to connect to UserSettings or UserSettingsInspector plugin");
                if (userSettings != nullptr) userSettings->Release();
                if (userSettingsInspector != nullptr) userSettingsInspector->Release();
                returnResponse(false);
            }

            returnResponse(true);
        }
    } // namespace Plugin
} // namespace WPEFramework
