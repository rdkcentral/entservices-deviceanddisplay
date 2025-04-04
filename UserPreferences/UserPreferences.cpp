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

            , _service(nullptr)
            , _notification(this)
            , _isMigrationDone(false)
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
            LOGINFO("dtor");
            ASSERT(nullptr == _service);
        }

        /**
        * @brief Converts a UI language format string to a presentation format string.
        *
        * @param[in]  inputLanguage         Input string in UI language format (e.g., "US_en").
        *                                    Expected format: "US_en", where:
        *                                    - The first two characters represent a country code (not validated).
        *                                    - The last two characters represent a language code (not validated).
        *                                    - The separator must be an underscore '_'.
        *                                    - The total length must be exactly 5 characters.
        *                                    - The function does not enforce that the language code is in the expected position.
        *
        * @param[out] presentationLanguage  Output string in presentation format (e.g., "en-US").
        *                                    If the input is valid, the format will be transformed to "en-US".
        *                                    If the input is invalid, the output string remains unchanged.
        *
        * @return True if the conversion was successful and the input format is valid.
        *         False if the input format is incorrect (e.g., missing separator, incorrect length).
        */

        bool UserPreferences::ConvertToUserSettingsFormat(const string& uiLanguage, string& presentationLanguage) {
            size_t sep = uiLanguage.find('_');
            if (sep == LANGUAGE_CODE_SEPARATOR_POS && uiLanguage.length() == LANGUAGE_CODE_LENGTH) {
                presentationLanguage = uiLanguage.substr(sep + 1) + "-" + uiLanguage.substr(0, sep);
                LOGINFO("Converting UI language '%s' to presentation format '%s'", uiLanguage.c_str(), presentationLanguage.c_str());
                return true;
            }
            LOGERR("Invalid UI language format: %s", uiLanguage.c_str());
            return false;
        }

        /**
        * @brief Converts a presentation format language string to a UI language format string.
        *
        * @param[in]  presentationLanguage  Input string in presentation format (e.g., "en-US").
        *                                    Expected format: "en-US", where:
        *                                    - The first two characters represent a language code.
        *                                    - The last two characters represent a country code.
        *                                    - The separator must be a hyphen '-'.
        *                                    - The total length must be exactly 5 characters.
        *
        * @param[out] uiLanguage            Output string in UI format (e.g., "US_en").
        *                                    If the input is valid, the format will be transformed to "US_en".
        *                                    If the input is invalid, the output string remains unchanged.
        *
        * @return True  - If the input string is in a valid presentation format and successfully converted.
        *         False - If the input string does not meet the expected format (e.g., incorrect length,
        *                 missing or misplaced separator). In this case, no conversion is performed.
        */

        bool UserPreferences::ConvertToUserPrefsFormat(const string& presentationLanguage, string& uiLanguage) {
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
        bool UserPreferences::RegisterForUserSettingsNotifications(Exchange::IUserSettings* userSettings) {
            if (nullptr == userSettings) {
                LOGERR("UserSettings interface is null; cannot register for notifications");
                return false;
            }
        
            userSettings->Register(&_notification);

            LOGINFO("Successfully registered for UserSettings notifications");
            return true;
        }

        // New function to handle migration logic
        bool UserPreferences::PerformMigration(Exchange::IUserSettings* userSettings) {
            

            if (userSettings == nullptr) {
                LOGERR("Failed to get UserSettings interface for migration");
                return false;
            }

            Exchange::IUserSettingsInspector* userSettingsInspector = _service->QueryInterfaceByCallsign<Exchange::IUserSettingsInspector>("org.rdk.UserSettings");
            if (userSettingsInspector == nullptr) {
                LOGERR("Failed to get UserSettingsInspector interface for migration");
                return false;
            }
        
            bool requiresMigration = false;
            uint32_t status = userSettingsInspector->GetMigrationState(Exchange::IUserSettingsInspector::SettingsKey::PRESENTATION_LANGUAGE, requiresMigration);
            if (status != Core::ERROR_NONE) {
                LOGERR("Failed to get migration state: %u", status);
                userSettingsInspector->Release();
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
                        if (ConvertToUserSettingsFormat(uiLanguage, presentationLanguage)) {
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
                    // Case 2: Migration needed - File doesn't exist
                } else if (error != nullptr && error->code == G_FILE_ERROR_NOENT) {
                    // Get current value from UserSettings and create new file
                    string presentationLanguage;
                    status = userSettings->GetPresentationLanguage(presentationLanguage);
                    if (status == Core::ERROR_NONE) {
                        string uiLanguage;
                        if (ConvertToUserPrefsFormat(presentationLanguage, uiLanguage)) {
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

            // Case 3: If there is no "/opt/user_preferences.conf" file, get value from user settings and, create and update "/opt/user_preferences.conf".
           // This handles cases where UserSettings was initialized earlier
                string presentationLanguage;
                status = userSettings->GetPresentationLanguage(presentationLanguage);
                if (status == Core::ERROR_NONE) {
                    string uiLanguage;
                    if (ConvertToUserPrefsFormat(presentationLanguage, uiLanguage)) {
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
        /*No need to perform AdminLock() for the IShell pointer, as Thunder guarantees 
        * that Initialize() will not be called unless the plugin has been successfully activated..*/

        const string UserPreferences::Initialize(PluginHost::IShell* shell) {
            LOGINFO("Initializing UserPreferences plugin");
            ASSERT(shell != nullptr);

            _service = shell;
            _service->AddRef();

            Exchange::IUserSettings* userSettings = nullptr;
            int count = 0;
            const int RETRY_COUNT = 5;
            const int RETRY_INTERVAL_MS = 1000;

           
            do {
                userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
                if (userSettings) {
                    userSettings->AddRef();
                    LOGINFO("Successfully obtained UserSettings interface after %d retries", count);
                    break;
                } else {
                    count++;
                    LOGERR("Failed to get UserSettings interface, retry: %d/%d", count, RETRY_COUNT);
                    usleep(RETRY_INTERVAL_MS * 1000);
                }
            } while (count < RETRY_COUNT);
           

            if (userSettings == nullptr) {
                LOGERR("Failed to obtain UserSettings interface after %d retries", RETRY_COUNT);
                return " ";
            }

            if (!_isMigrationDone && !PerformMigration(userSettings)) {
                return " ";
            }
        
            if (!RegisterForUserSettingsNotifications(userSettings)) {
                return " ";
            }
            userSettings->Release();

            return {};
        }

        void UserPreferences::Deinitialize(PluginHost::IShell* /* service */) {
            LOGINFO("Deinitialize");
            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
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

            UserPreferences::_instance = nullptr;
        }

        string UserPreferences::Information() const {
            return {};
        }

        /* 
        * Executing in the UserSettings notification context because:
        * 1. This callback is triggered only when the presentation language is changed, 
        *    which is a relatively infrequent operation and not performance critical.
        * 2. The handler does not make any blocking or recursive calls back into UserSettings, 
        *    so there is no risk of deadlock or circular dependency.
        *
        * Therefore, there's no need to offload this work to another thread. 
        * Handling it directly in the caller context ensures simplicity, avoids unnecessary thread management, 
        * and is safe within the constraints of this use case.
        */

        void UserPreferences::Notification::OnPresentationLanguageChanged(const string& language) {
             _parent->OnPresentationLanguageChanged(language);
        }

        void UserPreferences::OnPresentationLanguageChanged(const string& language) {
            LOGINFO("Presentation language changed to: %s", language.c_str());
            string uiLanguage;
            if (ConvertToUserPrefsFormat(language, uiLanguage)) {
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
            _adminLock.Lock();
            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            _adminLock.Unlock();
            if (!_isMigrationDone && !PerformMigration(userSettings)) {
                LOGERR("Migration failed; cannot get UI language");
                returnResponse(false);
            }

            if (!RegisterForUserSettingsNotifications(userSettings)) {
                LOGERR("Failed to register for UserSettings notifications; cannot get UI language");
                returnResponse(false);
            }

            string language;
            if (userSettings != nullptr) {
                string presentationLanguage;
                uint32_t status = userSettings->GetPresentationLanguage(presentationLanguage);
                if (status == Core::ERROR_NONE) {
                    if (!ConvertToUserPrefsFormat(presentationLanguage, language)) {
                        
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
                
            } else {
                LOGERR("Failed to get UserSettings interface");
                returnResponse(false);
            }
            userSettings->Release();

            response[SETTINGS_FILE_KEY] = language;
            returnResponse(true);
        }

          

        uint32_t UserPreferences::setUILanguage(const JsonObject& parameters, JsonObject& response) {
            
            LOGINFOMETHOD();
            _adminLock.Lock();

            Exchange::IUserSettings* userSettings = _service->QueryInterfaceByCallsign<Exchange::IUserSettings>("org.rdk.UserSettings");
            _adminLock.Unlock();
            if (!_isMigrationDone && !PerformMigration(userSettings)) {
                LOGERR("Migration failed; cannot set UI language");
                returnResponse(false);
            }

            if (!RegisterForUserSettingsNotifications(userSettings)) {
                LOGERR("Failed to register for UserSettings notifications; cannot set UI language");
                returnResponse(false);
            }

            returnIfStringParamNotFound(parameters, SETTINGS_FILE_KEY);
            string uiLanguage = parameters[SETTINGS_FILE_KEY].String();

            
            if (userSettings != nullptr) {
                string presentationLanguage;
                if (!ConvertToUserSettingsFormat(uiLanguage, presentationLanguage)) {
                    
                    returnResponse(false);
                }
                uint32_t status = userSettings->SetPresentationLanguage(presentationLanguage);
                if (status != Core::ERROR_NONE) {
                    LOGERR("Failed to set presentation language in UserSettings: %u", status);
                    
                    returnResponse(false);
                }
                
            } else {
                LOGERR("Failed to get UserSettings interface");
                returnResponse(false);
            }
            userSettings->Release();
            returnResponse(true);
        }
        //End methods

        //Begin events
        //End events

    } // namespace Plugin
} // namespace WPEFramework
