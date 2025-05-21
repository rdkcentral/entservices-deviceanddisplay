/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */
#include "plat_power.h"

#include "UtilsLogging.h"

#include "Settings.h"

class SettingsV1 {

    using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
    static constexpr const uint32_t UIMGR_SETTINGS_MAGIC = 0xFEBEEFAC;
    static constexpr const uint32_t PADDING_SIZE = 32;

    /*LED settings*/
    typedef struct _PWRMgr_LED_Settings_t {
        unsigned int brightness;
        unsigned int color;
    } PWRMgr_LED_Settings_t;

    typedef struct _PWRMgr_Settings_t {
        uint32_t magic;
        uint32_t version;
        uint32_t length;
        volatile PWRMgr_PowerState_t powerState;
        PWRMgr_LED_Settings_t ledSettings;
        uint32_t deep_sleep_timeout;
        bool nwStandbyMode;
        char padding[PADDING_SIZE];
    } PWRMgr_Settings_t;

    static PWRMgr_PowerState_t conv(const PowerState powerState)
    {
        PWRMgr_PowerState_t pwrState = PWRMGR_POWERSTATE_MAX;
        switch (powerState) {
        case PowerState::POWER_STATE_OFF:
            pwrState = PWRMGR_POWERSTATE_OFF;
            break;
        case PowerState::POWER_STATE_ON:
            pwrState = PWRMGR_POWERSTATE_ON;
            break;
        case PowerState::POWER_STATE_STANDBY:
            pwrState = PWRMGR_POWERSTATE_STANDBY;
            break;
        case PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP:
            pwrState = PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP;
            break;
        case PowerState::POWER_STATE_STANDBY_DEEP_SLEEP:
            pwrState = PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP;
            break;
        case PowerState::POWER_STATE_UNKNOWN:
        default:
            pwrState = PWRMGR_POWERSTATE_MAX;
            break;
        }
        return pwrState;
    }

public:
    inline static constexpr size_t Size()
    {
        return sizeof(PWRMgr_Settings_t) - PADDING_SIZE;
    }

    static bool Load(int fd, const Settings::Header& header, Settings& settings)
    {
        bool ok = false;
        struct stat buf = {};
        PWRMgr_Settings_t pwrSettings = {};
        const auto read_size = Size();

        if (header.length == read_size) {
            LOGINFO("[PwrMgr] Length of Persistence matches with Current Data Size \r\n");
            lseek(fd, 0, SEEK_SET);
            const auto ret = read(fd, &pwrSettings, read_size);
            if (ret != read_size) {
                // failure
                LOGERR("Unable to read full length");
            } else {
                LOGINFO("Persisted deep_sleep_delay = %d Secs \r\n", pwrSettings.deep_sleep_timeout);

                LOGINFO("Persisted network standby mode is: %s \r\n", pwrSettings.nwStandbyMode ? ("Enabled") : ("Disabled"));
#ifdef PLATCO_BOOTTO_STANDBY
                if (stat("/tmp/pwrmgr_restarted", &buf) != 0) {
                    settings._powerState = PowerState::POWER_STATE_STANDBY;
                    LOGINFO("Setting default powerstate to standby\n\r");
                }
#endif
                ok = true;
            }
        }

        return ok;
    }

    static bool Save(int fd, Settings& settings)
    {
        PWRMgr_Settings_t pwrSettings = {
            .magic = settings.magic(),
            .version = settings.version(),
            .length = Size(), // fixed for V1
            .powerState = conv(settings.powerState()),
            .ledSettings = {
                .brightness = 0, // unused, maintained for compatibility
                .color = 0       // unused, maintained for compatibility
            },
            .deep_sleep_timeout = settings.deepSleepTimeout(),
            .nwStandbyMode = settings.nwStandbyMode(),
            .padding = { 0 }
        };

        lseek(fd, 0, SEEK_SET);
        auto res = write(fd, &pwrSettings, Size());
        if (res < 0) {
            LOGERR("Failed to write settings %s", strerror(errno));
            return false;
        }
        return true;
    }

    static void initDefaults(Settings& settings)
    {
        LOGINFO("Initial Creation of UIMGR SettingsV1");
        settings._magic = UIMGR_SETTINGS_MAGIC;
        settings._version = static_cast<uint32_t>(Settings::Version::V1);
    }
};

using DefaultSettings = SettingsV1;

// Create initial settings
void Settings::initDefaults()
{
    DefaultSettings::initDefaults(*this);
}

Settings Settings::Load(const std::string& path)
{
    Settings settings{};
    struct stat buf = {};
    int fd = open(path.c_str(), O_CREAT | O_RDWR, S_IRWXU | S_IRUSR);
    bool ok = false;

    if (fd >= 0) {

        Header header;

        lseek(fd, 0, SEEK_SET);

        const int read_size = sizeof(Header);
        auto nbytes = read(fd, &header, read_size);

        if (nbytes == read_size) {
            switch (header.version) {
            case Version::V1:
                ok = SettingsV1::Load(fd, header, settings);
                break;
            default:
                LOGERR("Invalid version %d", header.version);
            }
        } else {
            // no data in settings file
#ifdef PLATCO_BOOTTO_STANDBY
            if (stat("/tmp/pwrmgr_restarted", &buf) != 0) {
                settings._powerStateBeforeReboot = PowerState::POWER_STATE_STANDBY;
                LOGINFO("Setting powerStateBeforeReboot to UNKNOWN, stat of /tmp/pwrmgr_restarted : %s", strerror(errno));
            }
#endif
        }

        if (!ok) {
            // failed to read settings file, init default settings
            settings.initDefaults();
            settings.save(fd);
        }

#ifdef PLATCO_BOOTTO_STANDBY
        if (stat("/tmp/pwrmgr_restarted", &buf) != 0) {
            settings._powerState = PowerState::POWER_STATE_STANDBY;
        }
#endif

        fsync(fd);
        close(fd);
    }
    return settings;
}

bool Settings::save(int fd)
{
    return DefaultSettings::Save(fd, *this);
}

bool Settings::Save(const std::string& path)
{
    int fd = open(path.c_str(), O_CREAT | O_WRONLY, S_IRWXU | S_IRUSR);

    if (fd < 0) {
        LOGERR("Failed to open settings file %s", strerror(errno));
        return false;
    }

    bool ok = save(fd);

    fsync(fd);
    close(fd);

    return ok;
}
