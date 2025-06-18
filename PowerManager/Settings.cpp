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
#include <iomanip> // for setfill
#include <sstream>

#include "plat_power.h"

#include "UtilsLogging.h"
#include "PowerUtils.h"

#include "Settings.h"

using util = PowerUtils;

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

    static PowerState conv(PWRMgr_PowerState_t state)
    {
        switch (state) {
        case PWRMGR_POWERSTATE_OFF:
            return PowerState::POWER_STATE_OFF;
        case PWRMGR_POWERSTATE_ON:
            return PowerState::POWER_STATE_ON;
        case PWRMGR_POWERSTATE_STANDBY:
            return PowerState::POWER_STATE_STANDBY;
        case PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP:
            return PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP;
        case PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP:
            return PowerState::POWER_STATE_STANDBY_DEEP_SLEEP;
        case PWRMGR_POWERSTATE_MAX:
        default:
            return PowerState::POWER_STATE_UNKNOWN;
        }
    }

public:
    inline static constexpr size_t Size()
    {
        return sizeof(PWRMgr_Settings_t) - PADDING_SIZE;
    }

    static bool Load(int fd, const Settings::Header& header, Settings& settings)
    {
        bool ok = false;
        PWRMgr_Settings_t pwrSettings = {};
        const auto expected_size = Size();

        if (header.length == expected_size) {
            lseek(fd, 0, SEEK_SET);
            const auto read_size = read(fd, &pwrSettings, expected_size);
            if (read_size != expected_size) {
                // failure
                LOGERR("Unable to read full length expected: %zu, actual %zu", expected_size, read_size);
            } else {

                settings._magic = pwrSettings.magic;
                settings._version = pwrSettings.version;
                settings._powerState = conv(pwrSettings.powerState);
                settings._deepSleepTimeout = pwrSettings.deep_sleep_timeout;
                settings._nwStandbyMode = pwrSettings.nwStandbyMode;

#ifdef PLATCO_BOOTTO_STANDBY
                struct stat buf = {};

                if (stat("/tmp/pwrmgr_restarted", &buf) != 0) {
                    settings._powerState = PowerState::POWER_STATE_STANDBY;
                    LOGINFO("Setting default powerstate to standby\n\r");
                }
#endif
                ok = true;
            }
        } else {
            LOGERR("Invalid header size expected: %zu, actual: %u", Size(), header.length);
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

using DefaultSettingsVersion = SettingsV1;

// Create initial settings
void Settings::initDefaults()
{
    DefaultSettingsVersion::initDefaults(*this);
}

Settings Settings::Load(const std::string& path)
{
    Settings settings{};
#ifdef PLATCO_BOOTTO_STANDBY
    struct stat buf = {};
#endif
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

        fsync(fd);
        close(fd);
    }

    if (!ok) {
        LOGERR("Failed to load settings file");

#ifdef PLATCO_BOOTTO_STANDBY
        // If we are in boot to standby mode, set powerState to standby
        if (stat("/tmp/pwrmgr_restarted", &buf) != 0) {
            LOGINFO("Boot to standby mode detected, setting powerState to UNKNOWN since PowerManager plugin was restarted");
            settings._powerState = PowerState::POWER_STATE_UNKNOWN;
        } else {
            LOGINFO("PowerManager Plugin was restarted");
        }
#endif
    }

    settings._powerStateBeforeReboot = settings._powerState;

    LOGINFO("Final settings: %s", settings.str().c_str());
    return settings;
}

bool Settings::save(int fd)
{
    return DefaultSettingsVersion::Save(fd, *this);
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

std::string Settings::str() const
{
    std::stringstream ss;

    ss << "magic: " << std::hex << _magic
       << "\n\tversion: " << _version
       << "\n\tpowerState: " << util::str(_powerState)
       << "\n\tpowerStateBeforeReboot " << util::str(_powerStateBeforeReboot)
       << "\n\tdeepsleep timeout sec: " << _deepSleepTimeout
       << "\n\tnwStandbyMode: " << (_nwStandbyMode ? "enabled" : "disabled");

    return ss.str();
}
