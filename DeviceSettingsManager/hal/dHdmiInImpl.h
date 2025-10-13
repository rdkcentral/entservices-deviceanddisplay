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
#pragma once

#include <cstdint>
#include "dHdmiIn.h"

#include "dsInternal.h"

#define TVSETTINGS_DALS_RFC_PARAM "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TvSettings.DynamicAutoLatency"

static int m_isInitialized = 0;
static int m_isPlatInitialized = 0;
static bool isDalsEnabled = 0;
//static dsHdmiInCap_t hdmiInCap_gs;
//static bool m_edidallmsupport[dsHDMI_IN_PORT_MAX];
//static bool m_vrrsupport[dsHDMI_IN_PORT_MAX];
//static bool m_hdmiPortVrrCaps[dsHDMI_IN_PORT_MAX];
//static tv_hdmi_edid_version_t m_edidversion[dsHDMI_IN_PORT_MAX];

static std::function<void(WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInPort, bool)> g_HdmiInHotPlugCallback;

class dHdmiInImpl : public hal::dHdmiIn::IPlatform {

    // delete copy constructor and assignment operator
    dHdmiInImpl(const dHdmiInImpl&) = delete;
    dHdmiInImpl& operator=(const dHdmiInImpl&) = delete;

public:
    dHdmiInImpl()
    {
        LOGINFO("dHdmiInImpl Constructor");
        LOGINFO("HDMI version: %s\n", HdmiConnectionToStrMapping[0].name);
        LOGINFO("HDMI version: %s\n", HdmiStatusToStrMapping[0].name);
        LOGINFO("HDMI version: %s\n", HdmiVerToStrMapping[0].name);
        //InitialiseHAL();
        // Initialize the platform
        /*pmStatus_t result = PLAT_INIT();
        if (PWRMGR_SUCCESS != result) {
            LOGERR("Failed to initialize power manager: %s", str(result));
        }*/
    }

    virtual ~dHdmiInImpl()
    {
        //InitialiseHAL();
        // Terminate the platform
        /*pmStatus_t result = PLAT_TERM();
        if (PWRMGR_SUCCESS != result) {
            LOGERR("Failed to terminate power manager: %s", str(result));
        }*/
    }

    void InitialiseHAL()
    {
        profile_t profileType = searchRdkProfile();
        getDynamicAutoLatencyConfig();
        if (PROFILE_TV == profileType)
        {
            LOGINFO("InitialiseHAL: its TV Profile");
            if (!m_isPlatInitialized)
            {
                /* Nexus init, if any here */
                dsError_t eError = dsHdmiInInit();
                LOGINFO("InitialiseHAL: dsHdmiInInit ret:%d", eError);
            }
            m_isPlatInitialized++;
        }
        m_isInitialized = 0;
        /*if (!m_isInitialized)
        {
            if (PROFILE_TV == profileType)
            {
                LOGINFO("its a TV Profile");
                dsHdmiInRegisterConnectCB(_dsHdmiInConnectCB);
            }
        }*/
    }

    void setAllCallbacks(const CallbackBundle bundle) override
    {
        if (bundle.OnHDMIInHotPlugEvent) {
            g_HdmiInHotPlugCallback = bundle.OnHDMIInHotPlugEvent;
            dsHdmiInRegisterConnectCB(DS_OnHDMIInHotPlugEvent);
        }

        LOGINFO("Set Callbacks");
    }

    profile_t searchRdkProfile(void) {
        LOGINFO("Entering searchRdkProfile");
        const char* devPropPath = "/etc/device.properties";
        char line[256], *rdkProfile = NULL;
        profile_t ret = PROFILE_INVALID;
        FILE* file;

        file = fopen(devPropPath, "r");
        if (file == NULL) {
            LOGINFO("searchRdkProfile: device.properties file not found.");
            return PROFILE_INVALID;
        }

        while (fgets(line, sizeof(line), file)) {
            rdkProfile = strstr(line, RDK_PROFILE);
            if (rdkProfile != NULL) {
                rdkProfile = strchr(line, '=');
                LOGINFO("searchRdkProfile: Found RDK_PROFILE");
                break;
            }
        }
        if(rdkProfile != NULL)
        {
            rdkProfile++; // Move past the '=' character
            if(0 == strncmp(rdkProfile, PROFILE_STR_TV, strlen(PROFILE_STR_TV)))
            {
                ret = PROFILE_TV;
            }
            else if (0 == strncmp(rdkProfile, PROFILE_STR_STB, strlen(PROFILE_STR_STB)))
            {
                ret = PROFILE_STB;
            }
        }
        else
        {
            LOGINFO("searchRdkProfile: NOT FOUND RDK_PROFILE in device properties file");
            ret = PROFILE_INVALID;
        }

        fclose(file);
        LOGINFO("Exit searchRdkProfile: RDK_PROFILE = %d", ret);
        return ret;
    }

    void getDynamicAutoLatencyConfig()
    {
        RFC_ParamData_t param = {0};
        WDMP_STATUS status = getRFCParameter((char*)"dssrv", TVSETTINGS_DALS_RFC_PARAM, &param);
        LOGINFO("DALS Feature Enable = [ %s ]", param.value);
        if(WDMP_SUCCESS == status && (strncasecmp(param.value,"true",4) == 0)) {
            isDalsEnabled = true;
            LOGINFO("Value of isDalsEnabled = [ %d ]", isDalsEnabled);
        }
        else {
            LOGINFO("Fetching RFC for DALS failed or DALS is disabled");
        }
    }

    static void DS_OnHDMIInHotPlugEvent(const dsHdmiInPort_t port, const bool isConnected)
    {
    if (g_HdmiInHotPlugCallback) {
        g_HdmiInHotPlugCallback(static_cast<WPEFramework::Exchange::IDeviceSettingsManagerHDMIIn::HDMIInPort>(port), isConnected);
    }
        LOGINFO("DS_OnHDMIInHotPlugEvent event Received: port=%d, isConnected=%s", port, isConnected ? "true" : "false");
    }

    virtual uint32_t GetHDMIInNumbefOfInputs(int32_t &count) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        return retCode;
    }
private:
};
