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

#define LANGUAGE_CODE_SEPARATOR_POS     2  // Position of separator ('_' or '-') in language codes
#define LANGUAGE_CODE_LENGTH            5  // Total length of language codes (e.g., "CA_en" or "en-CA")

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
            , userSettings(nullptr)
            , _service(nullptr)
            , _notification(this)
            , _isMigrationDone(false)
            , _isRegisteredForUserSettingsNotif(false)
            , _lastUILanguage("")
            ,_adminLock()
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

        /**
        * @brief Converts UI language format to presentation language format
        * 
        * @param Input string in UI language format (e.g., "US_en")
        *        Expected format: "US_en" where:
        *        - US = 2-character country code (not validated)
        *        - en = 2-character language code (not validated)
        *        - Separator is underscore '_'
        *        - Total length must be 5 characters
        * 
        * @param[out] presentationLanguage Output string in presentation format (e.g., "en-US")
        *        Format will be "en-US" if input is valid
        * 
        * 
        * @note We only validate the format/structure of the input string,
        *       not the actual country/language code values. This allows for flexibility
        *       in supporting new codes without code changes.
        */

        bool UserPreferences::ConvertToPresentationFormat(const string& uiLanguage, string& presentationLanguage) {
            size_t sep = uiLanguage.find('_');
            if (sep == LANGUAGE_CODE_SEPARATOR_POS && uiLanguage.length() == LANGUAGE_CODE_LENGTH) {
                presentationLanguage = uiLanguage.substr(sep + 1) + "-" + uiLanguage.substr(0, sep);
                LOGINFO("Converting UI language '%s' to presentation format '%s'", uiLanguage.c_str(), presentationLanguage.c_str());
                return true;
            }
            LOGERR("Invalid UI language format: %s", uiLanguage.c_str());
            return false;
        }

        bool UserPreferences::ConvertToUIFormat(const string& presentationLanguage, string& uiLanguage) {
            size_t sep = presentationLanguage.find('-');
            if (sep == LANGUAGE_CODE_SEPARATOR_POS && presentationLanguage.length() == LANGUAGE_CODE_LENGTH) {
                uiLanguage = presentationLanguage.substr(sep + 1) + "_" + presentationLanguage.substr(0, sep);
                LOGINFO("Converting presentation language '%s' to UI format '%s'",presentationLanguage.c_str(), uiLanguage.c_str());
                return true;
            }
            LOGERR("Invalid presentation language format: %s", presentationLanguage.c_str());
            return false;
        }

        // New function to handle UserSettings notification registration
        bool UserPreferences::RegisterForUserSettingsNotifications() {
            if (_isRegisteredForUserSettingsNotif) {
                LOGINFO("Already registered for UserSettings notifications");
                return true;
            }

            _adminLock.Lock();

            if (_service == nullptr) {
                LOGERR("Service not initialized; cannot register for notifications");
                return false;
            }

            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            _adminLock.Unlock();
            if (userSettings == nullptr) {
                LOGERR("Failed to get UserSettings interface for registration");
                return false;
            }
        
            userSettings->Register(&_notification);
            _isRegisteredForUserSettingsNotif = true;
            userSettings->Release();
            LOGINFO("Successfully registered for UserSettings notifications");
            return true;
        }

        // New function to handle migration logic
        bool UserPreferences::PerformMigration() {
            if (_isMigrationDone) {
                LOGINFO("Migration already completed");
                return true;
            }

            _adminLock.Lock();

            if (_service == nullptr) {
                LOGERR("Service not initialized; cannot perform migration");
                return false;
            }

            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            _adminLock.Unlock();
            if (userSettings == nullptr) {
                LOGERR("Failed to get UserSettings interface for migration");
                return false;
            }

            Exchange::IUserSettingsInspector* userSettingsInspector = _service->QueryInterfaceByCallsign<Exchange::IUserSettingsInspector>("org.rdk.UserSettings");
            if (userSettingsInspector == nullptr) {
                LOGERR("Failed to get UserSettingsInspector interface for migration");
                userSettings->Release();
                return false;
            }
        
            bool requiresMigration = false;
            uint32_t status = userSettingsInspector->GetMigrationState(Exchange::IUserSettingsInspector::SettingsKey::PRESENTATION_LANGUAGE, requiresMigration);
            if (status != Core::ERROR_NONE) {
                LOGERR("Failed to get migration state: %u", status);
                userSettingsInspector->Release();
                userSettings->Release();
                return false;
            }
        
            g_autoptr(GKeyFile) file = g_key_file_new();
            g_autoptr(GError) error = nullptr;
        
            if (requiresMigration) {
                LOGINFO("Migration is required for presentation language");
                
                // Case 1: Migration needed - File exists
                if (g_key_file_load_from_file(file, SETTINGS_FILE_NAME, G_KEY_FILE_NONE, &error)) {
                    // Read existing UI language from file and update UserSettings
                    g_autofree gchar *val = g_key_file_get_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, &error);
                    if (val != nullptr) {
                        string uiLanguage = val;
                        // Convert UI language to presentation language format
                        string presentationLanguage;
                        if (ConvertToPresentationFormat(uiLanguage, presentationLanguage)) {
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
                        string uiLanguage;
                        if (ConvertToUIFormat(presentationLanguage, uiLanguage)) {
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
                    string uiLanguage;
                    if (ConvertToUIFormat(presentationLanguage, uiLanguage)) {
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

            _isMigrationDone = true;
            userSettingsInspector->Release();
            userSettings->Release();
            LOGINFO("Migration completed successfully");
            return true;
        }

        const string UserPreferences::Initialize(PluginHost::IShell* shell) {
            LOGINFO("Initializing UserPreferences plugin");
            ASSERT(shell != nullptr);

            _service = shell;
            _service->AddRef();

            if (!RegisterForUserSettingsNotifications()) {
                return "Failed to initialize: Could not register for UserSettings notifications";
            }

            if (!PerformMigration()) {
                return "Failed to initialize: Migration failed";
            }

            return {};
        }

        void UserPreferences::Deinitialize(PluginHost::IShell* /* service */) {
            LOGINFO("Deinitialize");
            if (userSettings != nullptr) {
                userSettings->Unregister(&_notification);
                userSettings->Release();
                userSettings = nullptr;
            }
            _adminLock.Lock();
            if (_service != nullptr) {
                _service->Release();
                _service = nullptr;
            }
            _adminLock.Unlock();
            _isRegisteredForUserSettingsNotif = false;
            UserPreferences::_instance = nullptr;
        }

        void UserPreferences::Notification::OnPresentationLanguageChanged(const string& language) {
             _parent->OnPresentationLanguageChanged(language);
        }

        void UserPreferences::OnPresentationLanguageChanged(const string& language) {
            LOGINFO("Presentation language changed to: %s", language.c_str());
            string uiLanguage;
            if (ConvertToUIFormat(language, uiLanguage)) {
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
            if (!_isMigrationDone && !PerformMigration()) {
                LOGERR("Migration failed; cannot get UI language");
                returnResponse(false);
            }

            if (!_isRegisteredForUserSettingsNotif && !RegisterForUserSettingsNotifications()) {
                LOGERR("Failed to register for UserSettings notifications; cannot get UI language");
                returnResponse(false);
            }

            string language;
            _adminLock.Lock();
            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            _adminLock.Unlock();
            if (userSettings != nullptr) {
                string presentationLanguage;
                uint32_t status = userSettings->GetPresentationLanguage(presentationLanguage);
                if (status == Core::ERROR_NONE) {
                    if (!ConvertToUIFormat(presentationLanguage, language)) {
                        userSettings->Release();
                        returnResponse(false);
                    }
                    // Optimization: Update file only if language has changed
                    if (language != _lastUILanguage) {
                        g_autoptr(GKeyFile) file = g_key_file_new();
                        g_key_file_set_string(file, SETTINGS_FILE_GROUP, SETTINGS_FILE_KEY, (gchar*)language.c_str());
                        g_autoptr(GError) error = nullptr;
                        if (g_key_file_save_to_file(file, SETTINGS_FILE_NAME, &error)) {
                            _lastUILanguage = language;
                        } else {
                            LOGERR("Error saving file '%s': %s", SETTINGS_FILE_NAME, error->message);
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
            if (!_isMigrationDone && !PerformMigration()) {
                LOGERR("Migration failed; cannot set UI language");
                returnResponse(false);
            }

            if (!_isRegisteredForUserSettingsNotif && !RegisterForUserSettingsNotifications()) {
                LOGERR("Failed to register for UserSettings notifications; cannot set UI language");
                returnResponse(false);
            }

            returnIfStringParamNotFound(parameters, SETTINGS_FILE_KEY);
            string uiLanguage = parameters[SETTINGS_FILE_KEY].String();

            _adminLock.Lock();

            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            _adminLock.Unlock();
            if (userSettings != nullptr) {
                string presentationLanguage;
                if (!ConvertToPresentationFormat(uiLanguage, presentationLanguage)) {
                    userSettings->Release();
                    returnResponse(false);
                }
                uint32_t status = userSettings->SetPresentationLanguage(presentationLanguage);
                if (status != Core::ERROR_NONE) {
                    LOGERR("Failed to set presentation language in UserSettings: %u", status);
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
