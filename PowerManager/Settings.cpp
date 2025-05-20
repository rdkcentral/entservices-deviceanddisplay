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

#define PADDING_SIZE 32
#define _UIMGR_SETTINGS_MAGIC 0xFEBEEFAC

class SettingsV1 {
    using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

    typedef enum _UIDev_PowerState_t {
        UIDEV_POWERSTATE_OFF,
        UIDEV_POWERSTATE_STANDBY,
        UIDEV_POWERSTATE_ON,
        UIDEV_POWERSTATE_UNKNOWN
    } UIDev_PowerState_t;

    typedef struct _UIMgr_Settings_t {
        uint32_t magic;
        uint32_t version;
        uint32_t length;
        UIDev_PowerState_t powerState;
        char padding[PADDING_SIZE];
    } UIMgr_Settings_t;

    static PowerState conv(UIDev_PowerState_t powerState)
    {
        PowerState ret = PowerState::POWER_STATE_UNKNOWN;
        switch (powerState) {
        case UIDEV_POWERSTATE_OFF:
            ret = PowerState::POWER_STATE_OFF;
            break;
        case UIDEV_POWERSTATE_STANDBY:
            ret = PowerState::POWER_STATE_ON;
            break;
        case UIDEV_POWERSTATE_ON:
        case UIDEV_POWERSTATE_UNKNOWN:
            // To-do: legacy code mapping is incomplete
            break;
        }
        return ret;
    }

public:
    static bool Load(int fd, Settings& settings)
    {
        bool ok = false;
        UIMgr_Settings_t uiSettings;
        lseek(fd, 0, SEEK_SET);

        const auto read_size = sizeof(UIMgr_Settings_t);
        int ret = read(fd, &uiSettings, read_size);
        if (ret == read_size) {
            settings.setMagic(_UIMGR_SETTINGS_MAGIC);
            settings.setVersion(1);
            settings.SetPowerState(conv(uiSettings.powerState));
            // g_last_known_power_state = convert(pSettings->powerState);
#ifdef ENABLE_DEEP_SLEEP
            // To-do: deep sleep settings
            // settings._deep_sleep_timeout = deep_sleep_wakeup_timeout_sec;
#endif
            // To-do: network standby mode settings
            // settings._nwStandbyMode = nwStandbyMode_gs;
            ok = true;
        } else {
            LOGERR("Unable to read full length");
        }

        return ok;
    }
};

class SettingsV2 {

    using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

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
#ifdef ENABLE_DEEP_SLEEP
        uint32_t deep_sleep_timeout;
#endif
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
    inline static size_t Size()
    {
        return sizeof(PWRMgr_Settings_t) - PADDING_SIZE;
    }

    static bool Load(int fd, const Settings::Header& header, Settings& settings)
    {
        bool ok = false;
        PWRMgr_Settings_t pwrSettings = {};
        const auto read_size = sizeof(PWRMgr_Settings_t) - PADDING_SIZE;
        struct stat buf;

        if (header.length == read_size) {
            LOGINFO("[PwrMgr] Length of Persistence matches with Current Data Size \r\n");
            lseek(fd, 0, SEEK_SET);
            const auto ret = read(fd, &pwrSettings, read_size);
            if (ret != read_size) {
                // failure
                LOGERR("Unable to read full length");
            } else {
                // To-do: global
                // g_last_known_power_state = convert(pSettings->powerState);
#ifdef ENABLE_DEEP_SLEEP
                // To-do: global
                // deep_sleep_wakeup_timeout_sec = pSettings->deep_sleep_timeout;
                LOGINFO("Persisted deep_sleep_delay = %d Secs \r\n", pwrSettings.deep_sleep_timeout);
#endif
                // To-do: global
                // nwStandbyMode_gs = pwrMgrSettings.nwStandbyMode;

                // To-do: apis
                // PLAT_API_SetWakeupSrc(PWRMGR_WAKEUPSRC_LAN, nwStandbyMode_gs);
                // PLAT_API_SetWakeupSrc(PWRMGR_WAKEUPSRC_WIFI, nwStandbyMode_gs);

                LOGINFO("Persisted network standby mode is: %s \r\n", pwrSettings.nwStandbyMode ? ("Enabled") : ("Disabled"));
#ifdef PLATCO_BOOTTO_STANDBY
                if (stat("/tmp/pwrmgr_restarted", &buf) != 0) {
                    settings._powerState = PowerState::POWER_STATE_STANDBY;
                    LOGINFO("Setting default powerstate to standby\n\r");
                }
#endif
            }
        }

        return ok;
    }

    static bool Save(int fd, Settings& settings)
    {
        PWRMgr_Settings_t pwrSettings = {
            .magic = settings.magic(),
            .version = settings.version(),
            .length = sizeof(PWRMgr_Settings_t) - PADDING_SIZE, // fixed for V2
            .powerState = conv(settings.powerState()),
            .ledSettings = {
                .brightness = 0, // unused, maintained for compatibility
                .color = 0       // unused, maintained for compatibility
            },
#ifdef ENABLE_DEEP_SLEEP
            .deep_sleep_timeout = settings.deepSleepTimeout(),
#endif
            .nwStandbyMode = settings.nwStandbyMode(),
            .padding = { 0 }
        };

        lseek(fd, 0, SEEK_SET);
        auto res = write(fd, &pwrSettings, sizeof(PWRMgr_Settings_t));
        if (res < 0) {
            LOGERR("Failed to write settings %s", strerror(errno));
            return false;
        }
        return true;
    }
};

// Create initial settings
void Settings::initDefaults()
{
    LOGINFO("Initial Creation of UIMGR Settings\r\n");
    //struct stat buf;
    _magic = _UIMGR_SETTINGS_MAGIC;
    _version = 1;
    _powerState = PowerState::POWER_STATE_ON;
#ifdef ENABLE_DEEP_SLEEP
    // To-do:
    // pSettings->deep_sleep_timeout = deep_sleep_wakeup_timeout_sec;
#endif
    // To-do:
    // pSettings->nwStandbyMode = nwStandbyMode_gs;
#ifdef PLATCO_BOOTTO_STANDBY
    //if (stat("/tmp/pwrmgr_restarted", &buf) != 0)
        //pSettings->powerState = PWRMGR_POWERSTATE_STANDBY;
#endif
}

Settings Settings::Load(const std::string& path)
{
    Settings settings{};

    int ret = open(path.c_str(), O_CREAT | O_RDWR, S_IRWXU | S_IRUSR);
    int fd = ret;
    struct stat buf;
    bool ok = false;

    if (fd >= 0) {

        Header header;

        /*PWRMgr_Settings_t* pSettings = &m_settings;*/
        const int read_size = sizeof(Header);
        lseek(fd, 0, SEEK_SET);
        ret = read(fd, &header, read_size);
        if (ret == read_size) {
            switch (header.version) {
            case 0:
                ok = SettingsV1::Load(fd, settings);
                break;

            case 1:
                if (header.length == SettingsV2::Size()) {
                    ok = SettingsV2::Load(fd, header, settings);
                }
#if 0
                    else if (((header.length < SettingsV2::size()))) {
                    /* New Code reading the old version persistent file  information */
                    LOGINFO("[PwrMgr] Length of Persistence is less than  Current Data Size \r\n");
                    lseek(fd, 0, SEEK_SET);
                    read_size = pSettings->length;
                    ret = read(fd, pSettings, read_size);
                    if (ret != read_size) {
                        LOGINFO("[PwrMgr] Read Failed for Data Length %d \r\n", ret);
                        ret = 0; // error case, not able to read full length
                    } else {
                        /*TBD - The struct should be initialized first so that we dont need to add
                                 manually the new fields.
                         */
                        g_last_known_power_state = convert(pSettings->powerState);
                        lseek(fd, 0, SEEK_SET);
#ifdef ENABLE_DEEP_SLEEP
                        pSettings->deep_sleep_timeout = deep_sleep_wakeup_timeout_sec;
#endif
                        pSettings->nwStandbyMode = nwStandbyMode_gs;
#ifdef PLATCO_BOOTTO_STANDBY
                        if (stat("/tmp/pwrmgr_restarted", &buf) != 0) {
                            pSettings->powerState = PWRMGR_POWERSTATE_STANDBY;
                        }
#endif
                        pSettings->length = (sizeof(PWRMgr_Settings_t) - PADDING_SIZE);
                        LOGINFO("[PwrMgr] Write PwrMgr Settings File With Current Data Length %d \r\n", pSettings->length);
                        ret = write(fd, pSettings, pSettings->length);
                        if (ret != pSettings->length) {
                            LOGINFO("[PwrMgr] Write Failed For  New Data Length %d \r\n", ret);
                            ret = 0; // error case, not able to read full length
                        }
                    }
                } else if (((pSettings->length > (sizeof(PWRMgr_Settings_t) - PADDING_SIZE)))) {
                    /* Old Code reading the migrated new  version persistent file  information */
                    LOGINFO("[PwrMgr] Length of Persistence is more than  Current Data Size. \r\n");

                    lseek(fd, 0, SEEK_SET);
                    read_size = (sizeof(PWRMgr_Settings_t) - PADDING_SIZE);
                    ret = read(fd, pSettings, read_size);
                    if (ret != read_size) {
                        LOGINFO("[PwrMgr] Read Failed for Data Length %d \r\n", ret);
                        ret = 0; // error case, not able to read full length
                    } else {
                        /*Update the length and truncate the file */
                        g_last_known_power_state = convert(pSettings->powerState);
                        lseek(fd, 0, SEEK_SET);
                        pSettings->length = (sizeof(PWRMgr_Settings_t) - PADDING_SIZE);
                        LOGINFO("[PwrMgr] Write and Truncate  PwrMgr Settings File With Current  Data Length %d ........\r\n", pSettings->length);
                        ret = write(fd, pSettings, pSettings->length);
                        if (ret != pSettings->length) {
                            LOGINFO("[PwrMgr] Write Failed For  New Data Length %d \r\n", ret);
                            ret = 0; // error case, not able to read full length
                        } else {
                            /* Truncate the File information */
                            int fret = 0;
                            lseek(fd, 0, SEEK_SET);
                            fret = ftruncate(fd, pSettings->length);
                            if (fret != 0) {
                                LOGINFO("[PwrMgr] Truncate Failed For  New Data Length %d \r\n", fret);
                                ret = 0; // error case, not able to read full length
                            }
                        }
                    }

                } else {
                    ret = 0; // Version 1 but not with current size and data...
                }
#endif
                break;
            default:
                LOGERR("Invalid version %d", header.version);
            }
        } else {
            /* no data in settings file */
#ifdef PLATCO_BOOTTO_STANDBY
            if (stat("/tmp/pwrmgr_restarted", &buf) != 0) {
                //g_powerStateBeforeReboot = PowerState::POWER_STATE_UNKNOWN;
                LOGINFO("Setting powerStateBeforeReboot to UNKNOWN");
            }
#endif
        }

        if (!ok) {
            // failed to read settings file, init default settings
            settings.initDefaults();
            settings.save(fd);
        }

        // To-do: take care of this in PowerController
        // #ifdef ENABLE_DEEP_SLEEP
        //         /* If Persistent power mode is Deep Sleep
        //            start a thread to put box to deep sleep after specified time.
        //         */
        //         if (pSettings->powerState == PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP) {
        //             __TIMESTAMP();
        //             LOGINFO("Box Reboots with Deep Sleep mode.. Start a Event Time SOurce  .. \r\n");
        //             guint dsleep_bootup_timeout = 3600; // In sec
        //             // dsleep_bootup_event_src = g_timeout_add_seconds(dsleep_bootup_timeout, invoke_deep_sleep_on_bootup_timeout, pwrMgr_Gloop);
        //             __TIMESTAMP();
        //             LOGINFO("Added Time source %d to put the box to deep sleep after %d Sec.. \r\n", dsleep_bootup_event_src, dsleep_bootup_timeout);
        //         }
        // #endif

        //         g_powerStateBeforeReboot = g_last_known_power_state;
        //         __TIMESTAMP();
        //         LOGINFO("Setting PowerStateBeforeReboot %s", str(g_last_known_power_state));
        //
        //         /* Sync with platform if it is supported */
        //         {
        //             PWRMgr_PowerState_t state;
        //             ret = PLAT_API_GetPowerState(&state);
        //             if (ret == 0) {
        //                 if (pSettings->powerState == state) {
        //                     LOGINFO("PowerState is already sync'd with hardware to %d\r\n", state);
        //                 } else {
        //                     LOGINFO(" \n PowerState before sync hardware state %d with UIMGR to %d\r\n", state, pSettings->powerState);
        //                     ret = PLAT_API_SetPowerState((PWRMgr_PowerState_t)pSettings->powerState);
        //                     PLAT_API_GetPowerState((PWRMgr_PowerState_t*)&state);
        //                     LOGINFO(" \n PowerState after sync hardware state %d with UIMGR to %d\r\n", state, pSettings->powerState);
        //
        //                     if (state != pSettings->powerState) {
        //                         LOGINFO("CRITICAL ERROR: PowerState sync failed \r\n");
        //                         pSettings->powerState = state;
        //                     }
        //                 }
        //                 if (fd >= 0) {
        //                     lseek(fd, 0, SEEK_SET);
        //                     ret = write(fd, pSettings, pSettings->length);
        //                     if (ret < 0) {
        //                     }
        //                 }
        //             } else {
        //                 /* Use settings stored in uimgr file */
        //             }
        //         }

        //         IARM_Bus_PWRMgr_EventData_t _eventData;
        // #ifdef PLATCO_BOOTTO_STANDBY
        //         if (stat("/tmp/pwrmgr_restarted", &buf) != 0) {
        //             _eventData.data.state.curState = IARM_BUS_PWRMGR_POWERSTATE_OFF;
        //             _eventData.data.state.newState = IARM_BUS_PWRMGR_POWERSTATE_STANDBY;
        //         } else
        // #endif
        //         {
        //             _eventData.data.state.curState = (IARM_Bus_PowerState_t)g_last_known_power_state;
        //             _eventData.data.state.newState = (IARM_Bus_PowerState_t)pSettings->powerState;
        //         }
        //         LOGINFO("%s: Init setting powermode change from %d to %d \r\n", __FUNCTION__, _eventData.data.state.curState,
        //             _eventData.data.state.newState);
        //
        //         IARM_Bus_BroadcastEvent(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, (void*)&_eventData, sizeof(_eventData));
        //
        //         if (ret > 0 && ret < (int)sizeof(*pSettings)) {
        //             _DumpSettings(pSettings);
        //             /* @TODO: assert pSettings->length == FileLength */
        //         }
        //
        //         if (ret > (int)sizeof(*pSettings)) {
        //             LOGINFO("Error: Should not have read that much ! \r\n");
        //             /* @TODO: Error Handling */
        //             /* Init Settings to default, truncate settings to 0 */
        //         }

        fsync(fd);
        close(fd);
    }
    return settings;
}

bool Settings::save(int fd)
{
    return SettingsV2::Save(fd, *this);
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
