// Out-of-line virtual destructor definition for RTTI/typeinfo
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
#include "dFPD.h"
#include "deviceUtils.h"
#include "dsHdmiIn.h"
#include "dsError.h"
#include "dsHdmiInTypes.h"
#include "dsUtl.h"
#include "dsTypes.h"
#include "dsInternal.h"
#include "dsRpc.h"
#include "dsFPD.h"

#include <WPEFramework/interfaces/IDeviceSettingsManager.h>

#define RDK_DSHAL_NAME "libds-hal.so"

#define ENTRY_LOG LOGINFO("%d: Enter %s \n", __LINE__, __func__);
#define EXIT_LOG LOGINFO("%d: EXIT %s \n", __LINE__, __func__);

static int m_isInitialized = 0;
static int m_isPlatInitialized=0;

/** Structure that defines internal data base for the FP */
typedef struct _dsFPDSettings_t_
{   
    dsFPDBrightness_t brightness;
    dsFPDState_t state;
    dsFPDColor_t color;
}_FPDSettings_t;

static _FPDSettings_t srvFPDSettings[dsFPD_INDICATOR_MAX];

class dFPDImpl : public hal::dFPD::IPlatform {

    // delete copy constructor and assignment operator
    //dHdmiInImpl(const dHdmiInImpl&) = delete;
    //dHdmiInImpl& operator=(const dHdmiInImpl&) = delete;

public:
    dFPDImpl()
    {
        LOGINFO("dFPDImpl Constructor");
        /*LOGINFO("HDMI version: %s", HdmiConnectionToStrMapping[0].name);
        LOGINFO("HDMI version: %s", HdmiStatusToStrMapping[0].name);
        LOGINFO("HDMI version: %s", HdmiVerToStrMapping[0].name);*/
        InitialiseHAL();
        // Initialize the platform
        /*pmStatus_t result = PLAT_INIT();
        if (PWRMGR_SUCCESS != result) {
            LOGERR("Failed to initialize power manager: %s", str(result));
        }*/
    }

    virtual ~dFPDImpl()
    {
        LOGINFO("dFPDImpl Destructor");
        DeInitialiseHAL();
        // Terminate the platform
        /*pmStatus_t result = PLAT_TERM();
        if (PWRMGR_SUCCESS != result) {
            LOGERR("Failed to terminate power manager: %s", str(result));
        }*/
    }

    void InitialiseHAL()
    {
        LOGINFO("InitialiseHAL");
        if (!m_isInitialized) {
            for (int i = dsFPD_INDICATOR_MESSAGE; i < dsFPD_INDICATOR_MAX; i++)
            {
                srvFPDSettings[i].brightness = dsFPD_BRIGHTNESS_MAX;
                srvFPDSettings[i].state = dsFPD_STATE_OFF;
                            srvFPDSettings[i].color = dsFPD_COLOR_BLUE;
            }

            m_isInitialized = 1;

        }

        if (!m_isPlatInitialized) {
            LOGINFO("InitialiseHAL <dsFPD>");
            dsFPInit();
            m_isPlatInitialized = 1;
        }
    }

    void DeInitialiseHAL()
    {
        LOGINFO("DeInitialiseHAL");
        if (m_isPlatInitialized)
        {
            dsFPTerm();
            m_isPlatInitialized = 0;
        }
        m_isInitialized = 0;
    }

    /*static void* resolve(const std::string& libName, const std::string& symbolName) {
        void* handle = dlopen(libName.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "dlopen failed for " << libName << ": " << dlerror() << std::endl;
            return nullptr;
        }
        void* symbol = dlsym(handle, symbolName.c_str());
        if (!symbol) {
            std::cerr << "dlsym failed for " << symbolName << ": " << dlerror() << std::endl;
        }
        dlclose(handle);
        return symbol;
    }*/

    uint32_t SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        /*if (indicator < dsFPD_INDICATOR_MAX && brightNess <= dsFPD_BRIGHTNESS_MAX) {
            dsError_t eError = dsFPSetBrightness((dsFPDIndicator_t)indicator, (dsFPDBrightness_t)brightNess, persist);
            LOGINFO("SetFPDBrightness: indicator %d, brightNess %d, persist %d, eError %d", indicator, brightNess, persist, eError);
            if (eError == dsERR_NONE) {
                srvFPDSettings[indicator].brightness = brightNess;
                retCode = WPEFramework::Core::ERROR_NONE;
            }
        }*/
        return retCode;
    }
    private:
};
