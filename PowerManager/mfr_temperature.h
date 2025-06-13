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
 * @see the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @addtogroup HPK Hardware Porting Kit
 * @{
 * @par The Hardware Porting Kit
 * HPK is the next evolution of the well-defined Hardware Abstraction Layer
 * (HAL), but augmented with more comprehensive documentation and test suites
 * that OEM or SOC vendors can use to self-certify their ports before taking
 * them to RDKM for validation or to an operator for final integration and
 * deployment. The Hardware Porting Kit effectively enables an OEM and/or SOC
 * vendor to self-certify their own Video Accelerator devices, with minimal RDKM
 * assistance
 *
 */
/** @defgroup MFR MFR Module
 *  @{
 */
/** @defgroup MFR_HAL MFR HAL
 *  @{
 * @par Application API Specification
 * MFR HAL provides an interface for reading and writing device serialization information and doing image flashing operations
 */

/** @defgroup PLAT_MFR_DATA PLAT MFR DATA
 *  @{
 */

/**
 * @file mfrTypes.h
 * 
 * @brief MFR HAL header
 *
 * This file defines APIs, datatypes and error codes used by the MFR HAL
 *
 * @par Document
 * Document reference.
 *
 * @par Open Issues (in no particular order)
 * -# None
 *
 * @par Assumptions
 * -# None
 *
 * @par Abbreviations
 * - MFR:      Manufacturer library
 * - HDMI:     High-Definition multimedia Interface
 * - HDCP:     High-Bandwidth digital content protection
 * - MOCA:     Multimedia over coax alliance
 * - auth:     Authentication
 * - DTCP:     Digital transmission content protection
 * - CDL:      Code download
 * - RCDL:     Remote code download
 * - CA:       Certificate authority
 * - DVR:      Digital video recording
 * - SVN:      Software version number
 * - CRC:      Cyclic redundancy check
 * - oui:      Organizationally unique identifier
 * - DRI:      Disaster recovery image
 * - PDRI:     Peripheral disaster recovery image
 * - WIFI:     Wireless fidelity
 * - MAC:      Media access control address
 * - RF4CE:    Radio frequency for consumer electronics
 * - DTB:      Device tree binary
 * - PMI:      Product manufacturer information
 * - SOC:      System on chip
 * - TV:       Television
 * - BDRI:     Backup disaster recovery image
 * - CPD:      Critical panel data
 * - WB:       White balancing
 * - ALS:      Ambient light sensor
 * - LUX:      Unit of luminance or illumination of a one metre square area
 * - PCI:      Peripheral component interconnect
 * - AV:       Audio video
 * - TPV:      TPV technology limited
 * - FTA:      Factory test app
 * - WPS:      Wi-Fi protected setup
 */


#ifndef _MFR_TYPES_H
#define _MFR_TYPES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


/**
 * @brief MFR status codes.
 * 
 */
typedef enum _mfrError_t
{
    mfrERR_NONE = 0,                           ///< Input output operation is successful
    mfrERR_GENERAL = 0x1000,                   ///< Operation general error. This enum is deprecated
    mfrERR_INVALID_PARAM,                      ///< Invalid argument is passed to the module
    mfrERR_NOT_INITIALIZED,                    ///< Module is not initialised
    mfrERR_OPERATION_NOT_SUPPORTED,            ///< Not suppoted operation is performed
    mfrERR_UNKNOWN,                            ///< Unknown error. This enum is deprecated
    /* Please add Error Code here */
    mfrERR_MEMORY_EXHAUSTED,                   ///< Memory exhausted
    mfrERR_SRC_FILE_ERROR,                     ///< File related errors
    mfrERR_WRITE_FLASH_FAILED,                 ///< Flash write failed
    mfrERR_UPDATE_BOOT_PARAMS_FAILED,          ///< Boot params update failed
    mfrERR_FAILED_CRC_CHECK,                   ///< CRC check failed
    mfrERR_BAD_IMAGE_HEADER,                   ///< Bad image header error. Invalid Image(Not a valid image to flash in the partition)
    mfrERR_IMPROPER_SIGNATURE,                 ///< Improper signature error. Invalidate section data available in the image
    mfrERR_IMAGE_TOO_BIG,                      ///< Image too big error
    mfrERR_FAILED_INVALID_SIGNING_TIME,        ///< Invalid image signing time value
    mfrERR_FAILED_INVALID_SVN,                 ///< Invalid SVN error 
    mfrERR_FAILED_IMAGE_SIGNING_TIME_OLDER,    ///< Image signing time is older than expected. By comparing the signing time available in flash data with current image timing. return mfrERR_FAILED_IMAGE_SIGNING_TIME_OLDER flash signing image time is older
    mfrERR_FAILED_IMAGE_SVN_OLDER,             ///< SVN is older 
    mfrERR_FAILED_SAME_DRI_CODE_VERSION,       ///< Same DRI trying to write again. Current DRI image is requested to flash again. If curren image is corrupted this operation will corrupt the alternate bank also
    mfrERR_FAILED_SAME_PCI_CODE_VERSION,       ///< Same PCI trying to write again. Current PCI image is requested to flash again. If curren image is corrupted this operation will corrupt the alternate bank also
    mfrERR_IMAGE_FILE_OPEN_FAILED,             ///< Not able to open image file
    mfrERR_GET_FLASHED_IMAGE_DETAILS_FAILED,   ///< Not able to retrieve the flashed image details
    mfrERR_FLASH_VERIFY_FAILED,                ///< Not able to verify the flash
    mfrERR_ALREADY_INITIALIZED,                ///< Module already initialised
    mfrERR_FLASH_READ_FAILED,                  ///< Flash read failed
    mfrERR_FLASH_SOFT_LOCK_FAILED,             ///< Flash soft lock failed
    mfrERR_TEMP_READ_FAILED,                   ///< Temperature read failed
    mfrERR_MAX                                 ///< Out of range - required to be the last item of the enum
} mfrError_t;

/**
 * @brief Serialization data
 * 
 */
typedef struct _mfrSerializedData_t
{
    char * buf;                                ///< Buffer containing the data
    size_t bufLen;                             ///< Length of the data buffer
    void (* freeBuf) (char *buf);              ///< Function used to free the buffer. If NULL, the user does not need to free the buffer
} mfrSerializedData_t;

/**
 * @brief MFR image types
 * 
 */
typedef enum _mfrImageType_t
{
    mfrIMAGE_TYPE_CDL,                               ///< CDL image type
    mfrIMAGE_TYPE_RCDL,                              ///< RCDL image type.
    mfrUPGRADE_IMAGE_MONOLITHIC,                     ///< Monolithic image type
    mfrUPGRADE_IMAGE_PACKAGEHEADER,                  ///< Package header image type
    mfrIMAGE_TYPE_MAX,                               ///< Out of range - required to be the last item of the enum
} mfrImageType_t;


/**
 * @brief MFR image write progress status
 * 
 */
 typedef enum _mfrUpgradeProgress_t
 {
   mfrUPGRADE_PROGRESS_NOT_STARTED = 0,               ///< not started
   mfrUPGRADE_PROGRESS_STARTED,                       ///< in progress
   mfrUPGRADE_PROGRESS_ABORTED,                       ///< failed
   mfrUPGRADE_PROGRESS_VERIFYING,                     ///< Verifying
   mfrUPGRADE_PROGRESS_FLASHING,                      ///< Flashing
   mfrUPGRADE_PROGRESS_REBOOTING,                     ///< Rebooting
   mfrUPGRADE_PROGRESS_COMPLETED,                     ///< success
   mfrUPGRADE_PROGRESS_MAX                            ///< Out of range - required to be the last item of the enum 
 } mfrUpgradeProgress_t;


/**
 * @brief MFR image upgrade status
 * 
 */
typedef struct _mfrUpgradeStatus_t
{
  mfrUpgradeProgress_t progress;                    ///< MFR upgrade progress status. @see mfrUpgradeProgress_t
  mfrError_t error;                                 ///< Error @see mfrError_t
  char error_string[32];                            ///< Error string
  int percentage;                                   ///< MFR upgrade percentage
} mfrUpgradeStatus_t;

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
#endif

/** @} */ // End of PLAT_MFR_DATA
/** @} */ // End of MFR_HAL
/** @} */ // End of MFR Module
/** @} */ // End of HPK
