/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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

/**
 * @defgroup iarmmgrs
 * @{
 * @defgroup mfr
 * @{
 **/


#ifndef __MFR_TEMPERATURE_API_H__
#define __MFR_TEMPERATURE_API_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "mfrTypes.h"

typedef enum _mfrTemperatureState_t {
    mfrTEMPERATURE_NORMAL = 0,    /* Temp is within normal operating range */
    mfrTEMPERATURE_HIGH,          /* Temp is high, but just a warning as device can still operate */
    mfrTEMPERATURE_CRITICAL       /* Temp is critical, should trigger a thermal reset */
} mfrTemperatureState_t;


/**
* @brief get current temperature of the core
*
* @param [out] state:  the current state of the core temperature
* @param [out] temperatureValue:  raw temperature value of the core
*              in degrees Celsius
*
* @return Error Code
*/
mfrError_t mfrGetTemperature(mfrTemperatureState_t *state, int *temperatureValue, int *wifiTemp);


/**
* @brief Set temperature thresholds which will determine the state returned
​​*        from a call to mfrGetTemperature
*
* @param [in] tempHigh:  Temperature threshold at which mfrTEMPERATURE_HIGH
*                        state will be reported.
* @param [in] tempCritical:  Temperature threshold at which mfrTEMPERATURE_CRITICAL
*                            state will be reported.
*
* @return Error Code
*/
mfrError_t mfrSetTempThresholds(int tempHigh, int tempCritical);


/**
* @brief Get the temperature thresholds which determine the state returned
​​*        from a call to mfrGetTemperature
*
* @param [out] tempHigh:  Temperature threshold at which mfrTEMPERATURE_HIGH
*                        state will be reported.
* @param [out] tempCritical:  Temperature threshold at which mfrTEMPERATURE_CRITICAL
*                            state will be reported.
*
* @return Error Code
*/
mfrError_t mfrGetTempThresholds(int *tempHigh, int *tempCritical);


#ifdef __cplusplus
}
#endif

#endif /*__MFR_TEMPERATURE_API_H__*/

/** @} */
/** @} */