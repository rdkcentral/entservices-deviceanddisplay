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

#include <stdint.h>
#include <core/Portability.h>
#include <interfaces/IPowerManager.h>

#include "libIBus.h"
#include "PowerUtils.h"
#include "UtilsLogging.h"

#include "Thermal.h"

typedef enum _PWRMgr_ThermalState_t {
    PWRMGR_TEMPERATURE_NORMAL = 0,
    PWRMGR_TEMPERATURE_HIGH,
    PWRMGR_TEMPERATURE_CRITICAL
} PWRMgr_ThermalState_t;

class ThermalImpl : public hal::Thermal::IPlatform {
    using ThermalTemperature = WPEFramework::Exchange::IPowerManager::ThermalTemperature;

public:
    ThermalImpl() = default;
    ~ThermalImpl() = default;

    virtual uint32_t GetTemperatureThresholds(float &tempHigh,float &tempCritical) const override
    {
        uint32_t result = WPEFramework::Core::ERROR_GENERAL;

        IARM_Result_t iarm_result = IARM_RESULT_IPCCORE_FAIL;
        IARM_Bus_MFRLib_ThermalSoCTemp_Param_t param = {};

        iarm_result = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_GetTemperatureThresholds, (void *)&param, sizeof(IARM_Bus_MFRLib_ThermalSoCTemp_Param_t));

        if (IARM_RESULT_SUCCESS == iarm_result) {
        LOGINFO("Success IARM_BUS_MFRLIB_API_GetTemperatureThresholds\n");
            result = WPEFramework::Core::ERROR_NONE;
            tempHigh = param.highTemp;
            tempCritical = param.criticalTemp;
            LOGINFO("Received High Temperature Threshold as : %0.6f and Critical Temperature Threshold as : %0.6f \n",tempHigh ,tempCritical);
        } else {
            LOGERR("Failed IARM_BUS_MFRLIB_API_GetTemperatureThresholds\n");
        }

        return result;
    }

    virtual uint32_t SetTemperatureThresholds(float tempHigh,float tempCritical) override
    {
        uint32_t result = WPEFramework::Core::ERROR_GENERAL;

        IARM_Result_t iarm_result = IARM_RESULT_IPCCORE_FAIL;
        IARM_Bus_MFRLib_ThermalSoCTemp_Param_t param = {};

        param.highTemp = (int)tempHigh;
        param.criticalTemp = (int)tempCritical;

        LOGINFO("Setting High Temperature Threshold as : %0.6f and Critical Temperature Threshold as : %0.6f \n",(float)param.highTemp, (float)param.criticalTemp);

        iarm_result = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_SetTemperatureThresholds, (void *)&param, sizeof(IARM_Bus_MFRLib_ThermalSoCTemp_Param_t));

        if (IARM_RESULT_SUCCESS == iarm_result)
        {
            LOGINFO("Success IARM_BUS_MFRLIB_API_SetTemperatureThresholds\n");
            result = WPEFramework::Core::ERROR_NONE;
        }
        else
        {
            LOGERR("Failed IARM_BUS_MFRLIB_API_SetTemperatureThresholds\n");
        }

        return result;
    }    

    virtual uint32_t GetClockSpeed(uint32_t &speed) const override
    {
        uint32_t retValue = WPEFramework::Core::ERROR_GENERAL;
        IARM_Result_t iarm_result = IARM_RESULT_IPCCORE_FAIL;
        IARM_Bus_MFRLib_ThermalSoCFreq_Param_t param = {};

        iarm_result = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_GetCPUClockSpeed, (void *)&param, sizeof(IARM_Bus_MFRLib_ThermalSoCFreq_Param_t));
        if (IARM_RESULT_SUCCESS == iarm_result)
        {
            LOGINFO("Success IARM_BUS_MFRLIB_API_GetCPUClockSpeed\n");
            speed = param.cpu_clock_speed;
            retValue = WPEFramework::Core::ERROR_NONE;
            LOGINFO("Getting CPU Clock Speed  as [%u]\n",speed);
        }
        else
        {
            LOGERR("Failed IARM_BUS_MFRLIB_API_GetCPUClockSpeed\n");
        }

        return retValue;
    }

    virtual uint32_t SetClockSpeed(uint32_t speed) override
    {
        uint32_t retValue = WPEFramework::Core::ERROR_GENERAL;

        IARM_Result_t iarm_result = IARM_RESULT_IPCCORE_FAIL;
        IARM_Bus_MFRLib_ThermalSoCFreq_Param_t param = {};

        param.cpu_clock_speed = speed;
        LOGINFO("CPU Clock Speed Setting to [%u]\n",param.cpu_clock_speed);
        iarm_result = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_SetCPUClockSpeed, (void *)&param, sizeof(IARM_Bus_MFRLib_ThermalSoCFreq_Param_t));
        if (IARM_RESULT_SUCCESS == iarm_result)
        {
            LOGINFO("Success IARM_BUS_MFRLIB_API_SetCPUClockSpeed\n");
            retValue = WPEFramework::Core::ERROR_NONE;
        }
        else
        {
            LOGERR("Failed IARM_BUS_MFRLIB_API_SetCPUClockSpeed\n");
        }

        return retValue;
    }

    virtual uint32_t DetemineClockSpeeds(uint32_t &cpu_rate_Normal, uint32_t &cpu_rate_Scaled, uint32_t &cpu_rate_Minimal) override
    {
        uint32_t retValue = WPEFramework::Core::ERROR_GENERAL;

        IARM_Result_t iarm_result = IARM_RESULT_IPCCORE_FAIL;
        IARM_Bus_MFRLib_ThermalSoCFreq_Param_t param = {};

        iarm_result = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_SearchCPUClockSpeeds, (void *)&param, sizeof(IARM_Bus_MFRLib_ThermalSoCFreq_Param_t));
        if (IARM_RESULT_SUCCESS == iarm_result)
        {
            LOGINFO("Success IARM_BUS_MFRLIB_API_SearchCPUClockSpeeds\n");
            cpu_rate_Normal   =  param.cpu_rate_Normal;
            cpu_rate_Scaled   =  param.cpu_rate_Scaled;
            cpu_rate_Minimal  =  param.cpu_rate_Minimal;
            retValue = WPEFramework::Core::ERROR_NONE;
            LOGINFO("Available CPU Frequencies are: Normal:%u Scaled:%u Minimal:%u\n",cpu_rate_Normal ,cpu_rate_Scaled ,cpu_rate_Minimal);
        }
        else
        {
            LOGERR("Failed IARM_BUS_MFRLIB_API_SearchCPUClockSpeeds\n");
        }

        return retValue;
    }

    virtual uint32_t GetTemperature(mfrTemperatureState_t &curState, float &curTemperature, float &wifiTemperature) const override
    {
        uint32_t retValue = WPEFramework::Core::ERROR_GENERAL;

        IARM_Result_t iarm_result = IARM_RESULT_IPCCORE_FAIL;
        IARM_Bus_MFRLib_ThermalSoCTemp_Param_t param = {};

        iarm_result = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_GetTemperature, (void *)&param, sizeof(IARM_Bus_MFRLib_ThermalSoCTemp_Param_t));
        if (IARM_RESULT_SUCCESS == iarm_result) {
            curState = (mfrTemperatureState_t)param.curState;
            curTemperature = param.curSoCTemperature;
            wifiTemperature = param.curWiFiTemperature;
            retValue = WPEFramework::Core::ERROR_NONE;
            LOGINFO("SoC Temperature : %d and Wifi Temperature : %d\n",(int)(curTemperature), (int)(wifiTemperature));
        } else {
            LOGERR("Failed IARM_BUS_MFRLIB_API_GetTemperature\n");
        }

        return retValue;
    }

};

