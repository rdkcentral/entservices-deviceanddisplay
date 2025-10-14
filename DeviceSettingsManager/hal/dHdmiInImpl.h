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
#include "dHdmiIn.h"
#include "deviceUtils.h"
#include "dsHdmiIn.h"
#include "dsError.h"
#include "dsHdmiInTypes.h"
#include "dsUtl.h"
#include "dsTypes.h"
#include "dsInternal.h"
#include "dsRpc.h"

#include <WPEFramework/interfaces/IDeviceSettingsManager.h>

#define TVSETTINGS_DALS_RFC_PARAM "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TvSettings.DynamicAutoLatency"
#define RDK_DSHAL_NAME "libds-hal.so"

#define ENTRY_LOG LOGINFO("%d: Enter %s \n", __LINE__, __func__);
#define EXIT_LOG LOGINFO("%d: EXIT %s \n", __LINE__, __func__);

static int m_hdmiInInitialized = 0;
static int m_hdmiInPlatInitialized = 0;
static bool isDalsEnabled = 0;
static dsHdmiInCap_t hdmiInCap_gs;
static bool m_edidallmsupport[dsHDMI_IN_PORT_MAX];
static bool m_vrrsupport[dsHDMI_IN_PORT_MAX];
static bool m_hdmiPortVrrCaps[dsHDMI_IN_PORT_MAX];
static tv_hdmi_edid_version_t m_edidversion[dsHDMI_IN_PORT_MAX];

static std::function<void(WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort, bool)> g_HdmiInHotPlugCallback;
static std::function<void(WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort, WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInSignalStatus)> g_HdmiInSignalStatusCallback;
static std::function<void(WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort, WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIVideoPortResolution)> g_HdmiInVideoModeUpdateCallback;
static std::function<void(WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort, bool)> g_HdmiInAllmStatusCallback;
static std::function<void(WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort, WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInAviContentType)> g_HdmiInAviContentTypeCallback;
static std::function<void(int32_t, int32_t)> g_HdmiInAVLatencyCallback;
static std::function<void(WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort, WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInVRRType)> g_HdmiInVRRStatusCallback;
static std::function<void(WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort, bool)> g_HdmiInStatusCallback;

class dHdmiInImpl : public hal::dHdmiIn::IPlatform {

    // delete copy constructor and assignment operator
    //dHdmiInImpl(const dHdmiInImpl&) = delete;
    //dHdmiInImpl& operator=(const dHdmiInImpl&) = delete;

public:
    dHdmiInImpl()
    {
        LOGINFO("dHdmiInImpl Constructor");
        LOGINFO("HDMI version: %s", HdmiConnectionToStrMapping[0].name);
        LOGINFO("HDMI version: %s", HdmiStatusToStrMapping[0].name);
        LOGINFO("HDMI version: %s", HdmiVerToStrMapping[0].name);
        InitialiseHAL();
        // Initialize the platform
        /*pmStatus_t result = PLAT_INIT();
        if (PWRMGR_SUCCESS != result) {
            LOGERR("Failed to initialize power manager: %s", str(result));
        }*/
    }

    virtual ~dHdmiInImpl()
    {
        LOGERR("dHdmiInImpl Destructor");
        DeInitialiseHAL();
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
            if (!m_hdmiInPlatInitialized)
            {
                /* Nexus init, if any here */
                dsError_t eError = dsHdmiInInit();
                LOGINFO("InitialiseHAL: dsHdmiInInit ret:%d", eError);
            }
            m_hdmiInPlatInitialized++;
        }
    }

    void DeInitialiseHAL()
    {
        profile_t profileType = searchRdkProfile();
        getDynamicAutoLatencyConfig();

        LOGINFO("DeInitialiseHAL");
        if (PROFILE_TV == profileType)
        {
            LOGINFO("its TV Profile");
            if (m_hdmiInPlatInitialized)
            {
                m_hdmiInPlatInitialized--;
                if (!m_hdmiInPlatInitialized)
                {
                    dsHdmiInTerm();
                }
                m_hdmiInPlatInitialized = 0;
            }
        }
    }

    static void* resolve(const std::string& libName, const std::string& symbolName) {
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
    }

    bool getHdmiInPortPersistValue(const std::string& portName, int portIndex) {
        try {
            std::string value = device::HostPersistence::getInstance().getProperty(portName + ".edidallmEnable");
            bool support = (value == "TRUE");
            LOGINFO("Port %s: _EdidAllmSupport: %s , m_edidallmsupport: %d", portName.c_str(), value.c_str(), support);
            return support;
        } catch(...) {
            LOGINFO("Port %s: Exception in Getting the %s EDID allm support from persistence storage..... ", portName.c_str(), portName.c_str());
            return true;
        }
    }

    static dsError_t getVRRSupport (dsHdmiInPort_t iHdmiPort, bool *vrrSupport) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsHdmiInGetVRRSupport_t)(dsHdmiInPort_t iHdmiPort, bool *vrrSupport);
        static dsHdmiInGetVRRSupport_t dsHdmiInGetVRRSupportFunc = 0;

        if (dsHdmiInGetVRRSupportFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsHdmiInGetVRRSupportFunc = (dsHdmiInGetVRRSupport_t) dlsym(dllib, "dsHdmiInGetVRRSupport");
                if(dsHdmiInGetVRRSupportFunc == 0) {
                    LOGINFO("dsHdmiInGetVRRSupport (int,bool) is not defined %s", dlerror());
                }
                else {
                    LOGINFO("dsHdmiInGetVRRSupport loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsHdmiInGetVRRSupport  Opening RDK_DSHAL_NAME [%s] failed %s",
                    RDK_DSHAL_NAME, dlerror());
            }
        }
        if (0 != dsHdmiInGetVRRSupportFunc) {
            eRet = dsHdmiInGetVRRSupportFunc (iHdmiPort, vrrSupport);
            LOGINFO("dsHdmiInGetVRRSupportFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsHdmiInGetVRRSupportFunc = %p", dsHdmiInGetVRRSupportFunc);
        }
        return eRet;
    }

    static dsError_t setVRRSupport (dsHdmiInPort_t iHdmiPort, bool vrrSupport) {
        dsError_t eRet = dsERR_GENERAL;
        if (!m_hdmiPortVrrCaps[iHdmiPort]) {
            return dsERR_OPERATION_NOT_SUPPORTED;
        }
        typedef dsError_t (*dsHdmiInSetVRRSupport_t)(dsHdmiInPort_t iHdmiPort, bool vrrSupport);
        static dsHdmiInSetVRRSupport_t dsHdmiInSetVRRSupportFunc = 0;

        if (dsHdmiInSetVRRSupportFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsHdmiInSetVRRSupportFunc = (dsHdmiInSetVRRSupport_t) dlsym(dllib, "dsHdmiInSetVRRSupport");
                if(dsHdmiInSetVRRSupportFunc == 0) {
                    LOGINFO("dsHdmiInSetVRRSupport (int,bool) is not defined %s", dlerror());
                }
                else {
                    LOGINFO("dsHdmiInSetVRRSupport loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsHdmiInSetVRRSupport  Opening RDK_DSHAL_NAME [%s] failed %s",
                    RDK_DSHAL_NAME, dlerror());
            }
        }
        LOGINFO("setVRRSupport to ds-hal:  EDID VRR Bit: %d", vrrSupport);
        if (0 != dsHdmiInSetVRRSupportFunc) {
            eRet = dsHdmiInSetVRRSupportFunc (iHdmiPort, vrrSupport);
            LOGINFO("[srv] %s: dsHdmiInSetVRRSupportFunc eRet: %d", __FUNCTION__, eRet);
        }
        else {
            LOGINFO("%s:  dsHdmiInSetVRRSupportFunc = %p\n", __FUNCTION__, dsHdmiInSetVRRSupportFunc);
        }
        LOGINFO("setVRRSupport to ds-hal:  EDID VRR Bit: %d\n", vrrSupport);
        if (0 != dsHdmiInSetVRRSupportFunc) {
            eRet = dsHdmiInSetVRRSupportFunc (iHdmiPort, vrrSupport);
            LOGINFO("dsHdmiInSetVRRSupportFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsHdmiInSetVRRSupportFunc = %p", dsHdmiInSetVRRSupportFunc);
        }
        return eRet;
    }

    static dsError_t setEdid2AllmSupport (dsHdmiInPort_t iHdmiPort, bool allmSupport) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsSetEdid2AllmSupport_t)(dsHdmiInPort_t iHdmiPort, bool allmSupport);
        static dsSetEdid2AllmSupport_t dsSetEdid2AllmSupportFunc = 0;

        if (dsSetEdid2AllmSupportFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsSetEdid2AllmSupportFunc = (dsSetEdid2AllmSupport_t) dlsym(dllib, "dsSetEdid2AllmSupport");
                if(dsSetEdid2AllmSupportFunc == 0) {
                    LOGINFO("dsSetEdid2AllmSupport (int,bool) is not defined %s", dlerror());
                }
                else {
                    LOGINFO("dsSetEdid2AllmSupport loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsSetEdid2AllmSupport  Opening RDK_DSHAL_NAME [%s] failed %s",
                    RDK_DSHAL_NAME, dlerror());
            }
        }
        LOGINFO("setEdid2AllmSupport to ds-hal:  EDID Allm Bit: %d", allmSupport);
        if (0 != dsSetEdid2AllmSupportFunc) {
            eRet = dsSetEdid2AllmSupportFunc (iHdmiPort, allmSupport);
            LOGINFO("dsSetEdid2AllmSupportFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsSetEdid2AllmSupportFunc = %p", dsSetEdid2AllmSupportFunc);
        }
        return eRet;
    }

    static dsError_t isHdmiARCPort (int iPort, bool* isArcEnabled) {
        dsError_t eRet = dsERR_GENERAL; 

        typedef bool (*dsIsHdmiARCPort_t)(int iPortArg, bool *boolArg);
        static dsIsHdmiARCPort_t dsIsHdmiARCPortFunc = 0;
        if (dsIsHdmiARCPortFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsIsHdmiARCPortFunc = (dsIsHdmiARCPort_t) dlsym(dllib, "dsIsHdmiARCPort");
                if(dsIsHdmiARCPortFunc == 0) {
                    LOGINFO("dsIsHdmiARCPort (int) is not defined %s", dlerror());
                    eRet = dsERR_GENERAL;
                }
                else {
                    LOGINFO("dsIsHdmiARCPort dsIsHdmiARCPortFunc loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsIsHdmiARCPort  Opening RDK_DSHAL_NAME[%s] failed %s",
                    RDK_DSHAL_NAME, dlerror());  //CID 168096 - Print Args
                eRet = dsERR_GENERAL;
            }
        }
        if (0 != dsIsHdmiARCPortFunc) { 
            dsIsHdmiARCPortFunc (iPort, isArcEnabled);
            LOGINFO("dsIsHdmiARCPort port %d isArcEnabled:%d", iPort, *isArcEnabled);
        }
        else {
            LOGINFO("dsIsHdmiARCPort  dsIsHdmiARCPortFunc = %p", dsIsHdmiARCPortFunc);
        }
        return eRet;
    }

    static dsError_t setEdidVersion (dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t iEdidVersion) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsSetEdidVersion_t)(dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t iEdidVersion);
        static dsSetEdidVersion_t dsSetEdidVersionFunc = 0;
        char edidVer[2];
        sprintf(edidVer,"%d",iEdidVersion);

        if (dsSetEdidVersionFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsSetEdidVersionFunc = (dsSetEdidVersion_t) dlsym(dllib, "dsSetEdidVersion");
                if(dsSetEdidVersionFunc == 0) {
                    LOGINFO("dsSetEdidVersion (int) is not defined %s", dlerror());
                }
                else {
                    LOGINFO("dsSetEdidVersionFunc loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsSetEdidVersion  Opening RDK_DSHAL_NAME [%s] failed %s",
                    RDK_DSHAL_NAME, dlerror());
            }
        }

        if (0 != dsSetEdidVersionFunc) {
            eRet = dsSetEdidVersionFunc (iHdmiPort, iEdidVersion);
            if (eRet == dsERR_NONE) {
                switch (iHdmiPort) {
                    case dsHDMI_IN_PORT_0:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI0.edidversion", edidVer);
                        LOGINFO("Port %s: Persist EDID Version: %d", "HDMI0", iEdidVersion);
                        break;
                    case dsHDMI_IN_PORT_1:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI1.edidversion", edidVer);
                        LOGINFO("Port %s: Persist EDID Version: %d", "HDMI1", iEdidVersion);
                        break;
                    case dsHDMI_IN_PORT_2:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI2.edidversion", edidVer);
                        LOGINFO("Port %s: Persist EDID Version: %d", "HDMI2", iEdidVersion);
                        break;
                    case dsHDMI_IN_PORT_NONE:
                    case dsHDMI_IN_PORT_3:
                    case dsHDMI_IN_PORT_4:
                    case dsHDMI_IN_PORT_MAX:
                        break;
                }
            // Whenever there is a change in edid version to 2.0, ensure the edid allm support and edid vrr support is updated with latest value
        if(iEdidVersion == HDMI_EDID_VER_20)
            {
            LOGINFO("As the version is changed to 2.0, we are updating the allm bit and the vrr bit in edid");
            setEdid2AllmSupport(iHdmiPort,m_edidallmsupport[iHdmiPort]);
                    setVRRSupport(iHdmiPort,m_vrrsupport[iHdmiPort]);
            }
            }
            LOGINFO("dsSetEdidVersionFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsSetEdidVersionFunc = %p", dsSetEdidVersionFunc);
        }
        return eRet;
    }

    static dsError_t getEdidVersion (dsHdmiInPort_t iHdmiPort, int *iEdidVersion) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetEdidVersion_t)(dsHdmiInPort_t iHdmiPort, tv_hdmi_edid_version_t *iEdidVersion);
        static dsGetEdidVersion_t dsGetEdidVersionFunc = 0;
        if (dsGetEdidVersionFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsGetEdidVersionFunc = (dsGetEdidVersion_t) dlsym(dllib, "dsGetEdidVersion");
                if(dsGetEdidVersionFunc == 0) {
                    LOGINFO("dsGetEdidVersion (int) is not defined %s", dlerror());
                }
                else {
                    LOGINFO("dsGetEdidVersionFunc loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsGetEdidVersion  Opening RDK_DSHAL_NAME [%s] failed %s",
                    RDK_DSHAL_NAME, dlerror());
            }
        }
        if (0 != dsGetEdidVersionFunc) {
            tv_hdmi_edid_version_t EdidVersion;
            eRet = dsGetEdidVersionFunc (iHdmiPort, &EdidVersion);
            int tmp = static_cast<int>(EdidVersion);
            *iEdidVersion = tmp;
            LOGINFO("dsGetEdidVersionFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("%s:  dsGetEdidVersionFunc = %p", __FUNCTION__, dsGetEdidVersionFunc);
        }
        return eRet;
    }

    static dsError_t getAllmStatus (dsHdmiInPort_t iHdmiPort, bool *allmStatus) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetAllmStatus_t)(dsHdmiInPort_t iHdmiPort, bool *allmStatus);
        static dsGetAllmStatus_t dsGetAllmStatusFunc = 0;
        if (dsGetAllmStatusFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsGetAllmStatusFunc = (dsGetAllmStatus_t) dlsym(dllib, "dsGetAllmStatus");
                if(dsGetAllmStatusFunc == 0) {
                    LOGINFO("dsGetAllmStatus (int) is not defined %s", dlerror());
                }
                else {
                    LOGINFO("dsGetAllmStatusFunc loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsGetAllmStatus  Opening RDK_DSHAL_NAME [%s] failed %s",
                   RDK_DSHAL_NAME, dlerror());
            }
        }
        if (0 != dsGetAllmStatusFunc) {
            eRet = dsGetAllmStatusFunc (iHdmiPort, allmStatus);
            LOGINFO("dsGetAllmStatusFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsGetAllmStatusFunc = %p", dsGetAllmStatusFunc);
        }
        return eRet;
    }

    static dsError_t getSupportedGameFeaturesList (dsSupportedGameFeatureList_t *fList) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetSupportedGameFeaturesList_t)(dsSupportedGameFeatureList_t *fList);
        static dsGetSupportedGameFeaturesList_t dsGetSupportedGameFeaturesListFunc = 0;
        if (dsGetSupportedGameFeaturesListFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsGetSupportedGameFeaturesListFunc = (dsGetSupportedGameFeaturesList_t) dlsym(dllib, "dsGetSupportedGameFeaturesList");
                if(dsGetSupportedGameFeaturesListFunc == 0) {
                    LOGINFO("dsGetSupportedGameFeaturesList (int) is not defined %s", dlerror());
                }
                else {
                    LOGINFO("dsGetSupportedGameFeaturesList loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsGetSupportedGameFeaturesList  Opening RDK_DSHAL_NAME [%s] failed %s",
                    RDK_DSHAL_NAME, dlerror());
            }
        }
        if (0 != dsGetSupportedGameFeaturesListFunc) {
            eRet = dsGetSupportedGameFeaturesListFunc (fList);
            LOGINFO("dsGetSupportedGameFeaturesListFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsGetSupportedGameFeaturesListFunc = %p", dsGetSupportedGameFeaturesListFunc);
        }
        return eRet;
    }

    static dsError_t getAVLatency_hal (int *audio_latency, int *video_latency)
    {
    dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetAVLatency_t)(int *audio_latency, int *video_latency);
        static dsGetAVLatency_t dsGetAVLatencyFunc = 0;
        if (dsGetAVLatencyFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsGetAVLatencyFunc = (dsGetAVLatency_t) dlsym(dllib, "dsGetAVLatency");
                if(dsGetAVLatencyFunc == 0) {
                    LOGINFO("dsGetAVLatency (int) is not defined %s", dlerror());
                }
                else {
                    LOGINFO("dsGetAVLatencyFunc loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsGetAVLatency  Opening RDK_DSHAL_NAME [%s] failed %s",
                    RDK_DSHAL_NAME, dlerror());
            }
        }
        if (0 != dsGetAVLatencyFunc) {
            eRet = dsGetAVLatencyFunc (audio_latency, video_latency);
            LOGINFO("dsGetAVLatencyFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsGetAVLatencyFunc = %p", dsGetAVLatencyFunc);
        }
        return eRet;
    }

    static dsError_t getHdmiVersion (dsHdmiInPort_t iHdmiPort, dsHdmiMaxCapabilityVersion_t  *capversion) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetHdmiVersion_t)(dsHdmiInPort_t iHdmiPort, dsHdmiMaxCapabilityVersion_t  *capversion);
        static dsGetHdmiVersion_t dsGetHdmiVersionFunc = 0;
        if (dsGetHdmiVersionFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsGetHdmiVersionFunc = (dsGetHdmiVersion_t) dlsym(dllib, "dsGetHdmiVersion");
                if(dsGetHdmiVersionFunc == 0) {
                    LOGINFO("dsGetHdmiVersion (int) is not defined %s", dlerror());
                    eRet = dsERR_GENERAL;
                }
                else {
                    LOGINFO("dsGetHdmiVersionFunc loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGERR("dsGetHdmiVersion  Opening RDK_DSHAL_NAME [%s] failed %s",
                   RDK_DSHAL_NAME, dlerror());
                eRet = dsERR_GENERAL;
            }
        }
        if (0 != dsGetHdmiVersionFunc) {
            eRet = dsGetHdmiVersionFunc (iHdmiPort, capversion);
            LOGINFO("dsGetHdmiVersionFunc eRet: %d", eRet);
        }
        return eRet;
    }

    void setAllCallbacks(const CallbackBundle bundle) override
    {
        ENTRY_LOG;
        profile_t profileType = searchRdkProfile();
        LOGINFO("setAllCallbacks: profileType %d", profileType);
        if (!m_hdmiInInitialized) {
            LOGINFO("Trace - First time Initialization");
            if (PROFILE_TV == profileType)
            {
                LOGINFO("setAllCallbacks: its TV Profile");
                if (bundle.OnHDMIInHotPlugEvent) {
                    LOGINFO("HDMI In Hot Plug Event Callback Registered");
                    g_HdmiInHotPlugCallback = bundle.OnHDMIInHotPlugEvent;
                    dsHdmiInRegisterConnectCB(DS_OnHDMIInHotPlugEvent);
                }

                typedef dsError_t (*dsHdmiInRegisterSignalChangeCB_t)(dsHdmiInSignalChangeCB_t CBFunc);
                static dsHdmiInRegisterSignalChangeCB_t signalChangeCBFunc = 0;
                if (bundle.OnHDMIInSignalStatusEvent) {
                    LOGINFO("HDMI In Signal Status Event Callback Registered");
                    g_HdmiInSignalStatusCallback = bundle.OnHDMIInSignalStatusEvent;
                    if (!signalChangeCBFunc) {
                        signalChangeCBFunc = (dsHdmiInRegisterSignalChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterSignalChangeCB");
                    }
                    if (signalChangeCBFunc) {
                        signalChangeCBFunc(DS_OnHDMIInSignalStatusEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterSignalChangeCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterStatusChangeCB_t)(dsHdmiInStatusChangeCB_t CBFunc);
                static dsHdmiInRegisterStatusChangeCB_t StatusCBFunc = 0;
                if (bundle.OnHDMIInStatusEvent) {
                    LOGINFO("HDMI In Status Event Callback Registered");
                    g_HdmiInStatusCallback = bundle.OnHDMIInStatusEvent;
                    if (!StatusCBFunc) {
                        StatusCBFunc = (dsHdmiInRegisterStatusChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterStatusChangeCB");
                    }
                    if (StatusCBFunc) {
                        StatusCBFunc(DS_OnHDMIInStatusEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterStatusChangeCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterVideoModeUpdateCB_t)(dsHdmiInVideoModeUpdateCB_t CBFunc);
                static dsHdmiInRegisterVideoModeUpdateCB_t videoModeUpdateCBFunc = 0;
                if (bundle.OnHDMIInVideoModeUpdateEvent) {
                    LOGINFO("HDMI In Video Mode Update Event Callback Registered");
                    g_HdmiInVideoModeUpdateCallback = bundle.OnHDMIInVideoModeUpdateEvent;
                    if (!videoModeUpdateCBFunc) {
                        videoModeUpdateCBFunc = (dsHdmiInRegisterVideoModeUpdateCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterVideoModeUpdateCB");
                    }
                    if (videoModeUpdateCBFunc) {
                        videoModeUpdateCBFunc(DS_OnHDMIInVideoModeUpdateEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterVideoModeUpdateCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterAllmChangeCB_t)(dsHdmiInAllmChangeCB_t CBFunc);
                static dsHdmiInRegisterAllmChangeCB_t allmChangeCBFunc = 0;
                if (bundle.OnHDMIInAllmStatusEvent) {
                    LOGINFO("HDMI In ALLM Status Event Callback Registered");
                    g_HdmiInAllmStatusCallback = bundle.OnHDMIInAllmStatusEvent;
                    if (!allmChangeCBFunc) {
                        allmChangeCBFunc = (dsHdmiInRegisterAllmChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterAllmChangeCB");
                    }
                    if (allmChangeCBFunc) {
                        allmChangeCBFunc(DS_OnHDMIInAllmStatusEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterALLMChangeCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterVRRChangeCB_t)(dsHdmiInVRRChangeCB_t CBFunc);
                static dsHdmiInRegisterVRRChangeCB_t vrrChangeCBFunc = 0;
                if (bundle.OnHDMIInVRRStatusEvent) {
                    LOGINFO("HDMI In VRR Status Event Callback Registered");
                    g_HdmiInVRRStatusCallback = bundle.OnHDMIInVRRStatusEvent;
                    if (!vrrChangeCBFunc) {
                        vrrChangeCBFunc = (dsHdmiInRegisterVRRChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterVRRChangeCB");
                    }
                    if (vrrChangeCBFunc) {
                        vrrChangeCBFunc(DS_OnHDMIInVRRStatusEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterVRRChangeCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterAviContentTypeChangeCB_t)(dsHdmiInAviContentTypeChangeCB_t CBFunc);
                static dsHdmiInRegisterAviContentTypeChangeCB_t AviContentTypeChangeCBFunc = 0;
                if (bundle.OnHDMIInAVIContentTypeEvent) {
                    LOGINFO("HDMI In AVI Content Type Event Callback Registered");
                    g_HdmiInAviContentTypeCallback = bundle.OnHDMIInAVIContentTypeEvent;
                    if (!AviContentTypeChangeCBFunc) {
                        AviContentTypeChangeCBFunc = (dsHdmiInRegisterAviContentTypeChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterAviContentTypeChangeCB");
                    }
                    if (AviContentTypeChangeCBFunc) {
                        AviContentTypeChangeCBFunc(DS_OnHDMIInAVIContentTypeEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterAviContentTypeChangeCB");
                    }
                }

                typedef dsError_t (*dsHdmiInRegisterAVLatencyChangeCB_t)(dsAVLatencyChangeCB_t CBFunc);
                static dsHdmiInRegisterAVLatencyChangeCB_t AVLatencyChangeCBFunc = 0;
                if (bundle.OnHDMIInAVLatencyEvent) {
                    LOGINFO("HDMI In AV Latency Event Callback Registered");
                    g_HdmiInAVLatencyCallback = bundle.OnHDMIInAVLatencyEvent;
                    if (!AVLatencyChangeCBFunc) {
                        AVLatencyChangeCBFunc = (dsHdmiInRegisterAVLatencyChangeCB_t)resolve(RDK_DSHAL_NAME, "dsHdmiInRegisterAVLatencyChangeCB");
                    }
                    if (AVLatencyChangeCBFunc) {
                        AVLatencyChangeCBFunc(DS_OnHDMIInAVLatencyEvent);
                    } else {
                        LOGERR("Failed to resolve dsHdmiInRegisterAVLatencyChangeCB");
                    }
                }
            }
        }
        EXIT_LOG;
    }

    void getPersistenceValue() override
    {
        int itr = 0;
        bool isARCCapable = false;
        for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
            isARCCapable = false;
            isHdmiARCPort (itr, &isARCCapable);
            hdmiInCap_gs.isPortArcCapable[itr] = isARCCapable; 
        }

        std::string _EdidAllmSupport("TRUE");
        m_edidallmsupport[dsHDMI_IN_PORT_0] = getHdmiInPortPersistValue("HDMI0.edidallmEnable", dsHDMI_IN_PORT_0);
        m_edidallmsupport[dsHDMI_IN_PORT_1] = getHdmiInPortPersistValue("HDMI1.edidallmEnable", dsHDMI_IN_PORT_1);
        m_edidallmsupport[dsHDMI_IN_PORT_2] = getHdmiInPortPersistValue("HDMI2.edidallmEnable", dsHDMI_IN_PORT_2);

        std::string _VRRSupport("TRUE");
        m_vrrsupport[dsHDMI_IN_PORT_0] = getHdmiInPortPersistValue("HDMI0.vrrEnable", dsHDMI_IN_PORT_0);
        m_vrrsupport[dsHDMI_IN_PORT_1] = getHdmiInPortPersistValue("HDMI1.vrrEnable", dsHDMI_IN_PORT_1);
        m_vrrsupport[dsHDMI_IN_PORT_2] = getHdmiInPortPersistValue("HDMI2.vrrEnable", dsHDMI_IN_PORT_2);
        m_vrrsupport[dsHDMI_IN_PORT_3] = getHdmiInPortPersistValue("HDMI3.vrrEnable", dsHDMI_IN_PORT_3);

        std::string _EdidVersion("1");
        try {
            _EdidVersion = device::HostPersistence::getInstance().getProperty("HDMI0.edidversion");
            m_edidversion[dsHDMI_IN_PORT_0] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
        } catch(...) {
            try {
                LOGINFO("Port %s: Exception in Getting the HDMI0 EDID version from persistence storage. Try system default...", "HDMI0");
                _EdidVersion = device::HostPersistence::getInstance().getDefaultProperty("HDMI0.edidversion");
                m_edidversion[dsHDMI_IN_PORT_0] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
            }
            catch(...) {
                LOGINFO("Port %s: Exception in Getting the HDMI0 EDID version from system default.....", "HDMI0");
                m_edidversion[dsHDMI_IN_PORT_0] = HDMI_EDID_VER_20;
            }
        }

        try {
            _EdidVersion = device::HostPersistence::getInstance().getProperty("HDMI1.edidversion");
            m_edidversion[dsHDMI_IN_PORT_1] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
        } catch(...) {
            try {
                LOGINFO("Port %s: Exception in Getting the HDMI1 EDID version from persistence storage. Try system default...", "HDMI1");
                _EdidVersion = device::HostPersistence::getInstance().getDefaultProperty("HDMI1.edidversion");
                m_edidversion[dsHDMI_IN_PORT_1] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
            }
            catch(...) {
                LOGINFO("Port %s: Exception in Getting the HDMI1 EDID version from system default.....", "HDMI1");
                m_edidversion[dsHDMI_IN_PORT_1] = HDMI_EDID_VER_20;
            }
        }

        try {
            _EdidVersion = device::HostPersistence::getInstance().getProperty("HDMI2.edidversion");
            m_edidversion[dsHDMI_IN_PORT_2] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
        } catch(...) {
            try {
                LOGINFO("Port %s: Exception in Getting the HDMI2 EDID version from persistence storage. Try system default...", "HDMI2");
                _EdidVersion = device::HostPersistence::getInstance().getDefaultProperty("HDMI2.edidversion");
                m_edidversion[dsHDMI_IN_PORT_2] = static_cast<tv_hdmi_edid_version_t>(atoi (_EdidVersion.c_str()));
            }
            catch(...) {
                LOGINFO("Port %s: Exception in Getting the HDMI2 EDID version from system default.....", "HDMI2");
                m_edidversion[dsHDMI_IN_PORT_2] = HDMI_EDID_VER_20;
            }
        }

        for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
            if (getVRRSupport(static_cast<dsHdmiInPort_t>(itr), &m_hdmiPortVrrCaps[itr]) >= 0) {
                LOGINFO("Port HDMI%d: VRR capability : %d", itr, m_hdmiPortVrrCaps[itr]);
            }
        }
        for (itr = 0; itr < dsHDMI_IN_PORT_MAX; itr++) {
            if (setEdidVersion (static_cast<dsHdmiInPort_t>(itr), m_edidversion[itr]) >= 0) {
                LOGINFO("Port HDMI%d: Initialized EDID Version : %d", itr, m_edidversion[itr]);
            }
        }
        m_hdmiInInitialized = 1;

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
        LOGINFO("DS_OnHDMIInHotPlugEvent event Received: port=%d, isConnected=%s", port, isConnected ? "true" : "false");
        if (g_HdmiInHotPlugCallback) {
            g_HdmiInHotPlugCallback(static_cast<WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort>(port), isConnected);
        }
    }

    static void DS_OnHDMIInSignalStatusEvent(const dsHdmiInPort_t port, const dsHdmiInSignalStatus_t signalStatus)
    {
        LOGINFO("DS_OnHDMIInSignalStatusEvent event Received: port=%d, signalStatus=%d", port, signalStatus);
        if (g_HdmiInSignalStatusCallback) {
            g_HdmiInSignalStatusCallback(static_cast<HDMIInPort>(port), static_cast<HDMIInSignalStatus>(signalStatus));
        }
    }

    static void DS_OnHDMIInStatusEvent(const dsHdmiInStatus_t status)
    {
        LOGINFO("DS_OnHDMIInStatusEvent event Received: status=%d, isPresented=%s", status.activePort, status.isPresented ? "true" : "false");
        
        if (g_HdmiInStatusCallback) {
            g_HdmiInStatusCallback(static_cast<HDMIInPort>(status.activePort), status.isPresented);
        }
    }

    static void DS_OnHDMIInVideoModeUpdateEvent(const dsHdmiInPort_t port, const dsVideoPortResolution_t videoPortResolution)
    {
        LOGINFO("DS_OnHDMIInVideoModeUpdateEvent event Received: port=%d", port); // adjust as needed

        if (g_HdmiInVideoModeUpdateCallback) {
            HDMIVideoPortResolution res;
            // Copy/convert fields
            res.name = std::string(videoPortResolution.name); // convert char[] to std::string
            // Copy other fields if needed

            g_HdmiInVideoModeUpdateCallback(static_cast<HDMIInPort>(port), res);
        }
    }

    static void DS_OnHDMIInAllmStatusEvent(const dsHdmiInPort_t port, const bool allmStatus)
    {
        LOGINFO("DS_OnHDMIInAllmStatusEvent event Received: port=%d, allmStatus=%s", port, allmStatus ? "true" : "false");
        if (g_HdmiInAllmStatusCallback) {
            g_HdmiInAllmStatusCallback(static_cast<HDMIInPort>(port), allmStatus);
        }
    }

    static void DS_OnHDMIInAVIContentTypeEvent(const dsHdmiInPort_t port, const dsAviContentType_t aviContentType)
    {
        LOGINFO("DS_OnHDMIInAVIContentTypeEvent event Received: port=%d, aviContentType=%d", port, aviContentType);
        if (g_HdmiInAviContentTypeCallback) {
            g_HdmiInAviContentTypeCallback(static_cast<HDMIInPort>(port), static_cast<HDMIInAviContentType>(aviContentType));
        }
    }

    static void DS_OnHDMIInAVLatencyEvent(const int32_t audioDelay, const int32_t videoDelay)
    {
        LOGINFO("DS_OnHDMIInAVLatencyEvent event Received: audioDelay=%d, videoDelay=%d", audioDelay, videoDelay);
        if (g_HdmiInAVLatencyCallback) {
            g_HdmiInAVLatencyCallback(audioDelay, videoDelay);
        }
    }

    static void DS_OnHDMIInVRRStatusEvent(const dsHdmiInPort_t port, const dsVRRType_t vrrType)
    {
        LOGINFO("DS_OnHDMIInVRRStatusEvent event Received: port=%d, vrrType=%d", port, vrrType);
        if (g_HdmiInVRRStatusCallback) {
            g_HdmiInVRRStatusCallback(static_cast<HDMIInPort>(port), static_cast<HDMIInVRRType>(vrrType));
        }
    }

    virtual uint32_t GetHDMIInNumberOfInputs(int32_t &count) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        return retCode;
    }

    uint32_t GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        return retCode;
    }

    uint32_t GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        int vLatency = 0;
        int aLatency = 0;
        if (getAVLatency_hal(&aLatency, &vLatency) == dsERR_NONE) {
            audioLatency = static_cast<uint32_t>(aLatency);
            videoLatency = static_cast<uint32_t>(vLatency);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        bool status = false;
        if (getAllmStatus(hdmiPort, &status) == dsERR_NONE) {
            allmStatus = status;
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            allmSupport = m_edidallmsupport[hdmiPort];
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            if (setEdid2AllmSupport(hdmiPort, allmSupport) == dsERR_NONE) {
                m_edidallmsupport[hdmiPort] = allmSupport;
                std::string val = allmSupport ? "TRUE" : "FALSE";
                switch (hdmiPort) {
                    case dsHDMI_IN_PORT_0:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI0.edidallmEnable", val);
                        LOGINFO("Port %s: Persist EDID ALLM Support: %s", "HDMI0", val.c_str());
                        break;
                    case dsHDMI_IN_PORT_1:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI1.edidallmEnable", val);
                        LOGINFO("Port %s: Persist EDID ALLM Support: %s", "HDMI1", val.c_str());
                        break;
                    case dsHDMI_IN_PORT_2:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI2.edidallmEnable", val);
                        LOGINFO("Port %s: Persist EDID ALLM Support: %s", "HDMI2", val.c_str());
                        break;
                    case dsHDMI_IN_PORT_NONE:
                    case dsHDMI_IN_PORT_3:
                    case dsHDMI_IN_PORT_4:
                    case dsHDMI_IN_PORT_MAX:
                        break;
                }
                retCode = WPEFramework::Core::ERROR_NONE;
            }
        }
        return retCode;
    }

    uint32_t GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsSupportedGameFeatureList_t fList;
        std::vector<WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInGameFeatureList> features;
        if (getSupportedGameFeaturesList(&fList) == dsERR_NONE) {
            // Assuming fList.features[i].feature is a string or can be converted to string
            /*for (uint32_t i = 0; i < fList.count; i++) {
                WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInGameFeatureList feature;
                feature.gameFeature = std::string(fList.features[i].feature); // adjust if needed
                features.push_back(feature);
            }
            gameFeatureList = WPEFramework::Core::Service<RPC::IIteratorType<
                WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInGameFeatureList,
                WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::ID_DEVICESETTINGS_MANAGER_HDMIIN_GAMELIST_ITERATOR>>
                ::Create(features);
            retCode = WPEFramework::Core::ERROR_NONE;*/ // Temporarily disabling as dsSupportedGameFeatureList_t is not defined
        }
        return retCode;
    }

    uint32_t SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        dsVideoPlaneType_t videoType = static_cast<dsVideoPlaneType_t>(videoPlaneType);
        if (dsHdmiInSelectPort(hdmiPort, requestAudioMix, videoType, topMostPlane) == dsERR_NONE) {
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsVideoRect_t rect;
        rect.x = videoPosition.x;
        rect.y = videoPosition.y;
        rect.width = videoPosition.width;
        rect.height = videoPosition.height;
        if (dsHdmiInScaleVideo(rect.x, rect.y, rect.width, rect.height) == dsERR_NONE) {
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsVideoZoom_t zoom = static_cast<dsVideoZoom_t>(zoomMode);
        if (dsHdmiInSelectZoomMode(zoom) == dsERR_NONE) {
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    static dsError_t getEDIDBytesInfo (dsHdmiInPort_t iHdmiPort, unsigned char *edid, int *length) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetEDIDBytesInfo_t)(dsHdmiInPort_t iHdmiPort, unsigned char *edid, int *length);
        static dsGetEDIDBytesInfo_t dsGetEDIDBytesInfoFunc = 0;
        if (dsGetEDIDBytesInfoFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsGetEDIDBytesInfoFunc = (dsGetEDIDBytesInfo_t) dlsym(dllib, "dsGetEDIDBytesInfo");
                if(dsGetEDIDBytesInfoFunc == 0) {
                    LOGINFO("dsGetEDIDBytesInfo (int) is not defined %s", dlerror());
                    eRet = dsERR_GENERAL;
                }
                else {
                    LOGINFO("dsGetEDIDBytesInfoFunc loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsGetEDIDBytesInfo  Opening RDK_DSHAL_NAME [%s] failed %s",
                    RDK_DSHAL_NAME, dlerror());
                eRet = dsERR_GENERAL;
            }
        }
        if (0 != dsGetEDIDBytesInfoFunc) {
            LOGINFO("Entering dsGetEDIDBytesInfoFunc");
            eRet = dsGetEDIDBytesInfoFunc (iHdmiPort, edid, length);
            LOGINFO("dsGetEDIDBytesInfoFunc eRet: %d data len: %d", eRet, *length);
        }
        return eRet;
    }

    uint32_t GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, int edidBytes[]) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        /*dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (getEDIDBytesInfo(hdmiPort, edidBytes, &edidBytesLength) == dsERR_NONE) {
            retCode = WPEFramework::Core::ERROR_NONE;
        }*/
        return retCode;
    }

    static dsError_t getHDMISPDInfo (dsHdmiInPort_t iHdmiPort, unsigned char *spd) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsGetHDMISPDInfo_t)(dsHdmiInPort_t iHdmiPort, unsigned char *data);
        static dsGetHDMISPDInfo_t dsGetHDMISPDInfoFunc = 0;
        if (dsGetHDMISPDInfoFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsGetHDMISPDInfoFunc = (dsGetHDMISPDInfo_t) dlsym(dllib, "dsGetHDMISPDInfo");
                if(dsGetHDMISPDInfoFunc == 0) {
                    LOGINFO("dsGetHDMISPDInfo (int) is not defined %s", dlerror());
                    eRet = dsERR_GENERAL;
                }
                else {
                    LOGINFO("dsGetHDMISPDInfoFunc loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsGetHDMISPDInfo  Opening RDK_DSHAL_NAME [%s] failed %s",
                   RDK_DSHAL_NAME, dlerror());
                eRet = dsERR_GENERAL;
            }
        }
        if (0 != dsGetHDMISPDInfoFunc) {
            eRet = dsGetHDMISPDInfoFunc (iHdmiPort, spd);
            LOGINFO("dsGetHDMISPDInfoFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("dsGetHDMISPDInfoFunc = %p", dsGetHDMISPDInfoFunc);
        }
        return eRet;
    }

    uint32_t GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (getHDMISPDInfo(hdmiPort, spdBytes) == dsERR_NONE) {
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        int edidVer = 0;
        if (getEdidVersion(hdmiPort, &edidVer) == dsERR_NONE) {
            edidVersion = static_cast<HDMIInEdidVersion>(edidVer);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        tv_hdmi_edid_version_t edidVer = static_cast<tv_hdmi_edid_version_t>(edidVersion);
        if (setEdidVersion(hdmiPort, edidVer) == dsERR_NONE) {
            m_edidversion[hdmiPort] = edidVer;
            std::string val = std::to_string(static_cast<int>(edidVer));
            switch (hdmiPort) {
                case dsHDMI_IN_PORT_0:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI0.edidversion", val);
                    LOGINFO("Port %s: Persist EDID Version: %s", "HDMI0", val.c_str());
                    break;
                case dsHDMI_IN_PORT_1:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI1.edidversion", val);
                    LOGINFO("Port %s: Persist EDID Version: %s", "HDMI1", val.c_str());
                    break;
                case dsHDMI_IN_PORT_2:
                    device::HostPersistence::getInstance().persistHostProperty("HDMI2.edidversion", val);
                    LOGINFO("Port %s: Persist EDID Version: %s", "HDMI2", val.c_str());
                    break;
                case dsHDMI_IN_PORT_NONE:
                case dsHDMI_IN_PORT_3:
                case dsHDMI_IN_PORT_4:
                case dsHDMI_IN_PORT_MAX:
                    break;
            }
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsVideoPortResolution_t videoRes;
        if (dsHdmiInGetCurrentVideoMode(&videoRes) == dsERR_NONE) {
            videoPortResolution.name = std::string(videoRes.name);
            videoPortResolution.pixelResolution = static_cast<HDMIInTVResolution>(videoRes.pixelResolution);
            videoPortResolution.aspectRatio = static_cast<HDMIVideoAspectRatio>(videoRes.aspectRatio);
            videoPortResolution.stereoScopicMode = static_cast<HDMIInVideoStereoScopicMode>(videoRes.stereoScopicMode);
            videoPortResolution.frameRate = static_cast<HDMIInVideoFrameRate>(videoRes.frameRate);
            videoPortResolution.interlaced = videoRes.interlaced;
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        dsHdmiMaxCapabilityVersion_t capversion;
        if (getHdmiVersion(hdmiPort, &capversion) == dsERR_NONE) {
            capabilityVersion = static_cast<HDMIInCapabilityVersion>(capversion);
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    uint32_t SetVRRSupport(const HDMIInPort port, const bool vrrSupport) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            if (setVRRSupport(hdmiPort, vrrSupport) == dsERR_NONE) {
                m_vrrsupport[hdmiPort] = vrrSupport;
                std::string val = vrrSupport ? "TRUE" : "FALSE";
                switch (hdmiPort) {
                    case dsHDMI_IN_PORT_0:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI0.vrrEnable", val);
                        LOGINFO("Port %s: Persist VRR Support: %s", "HDMI0", val.c_str());
                        break;
                    case dsHDMI_IN_PORT_1:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI1.vrrEnable", val);
                        LOGINFO("Port %s: Persist VRR Support: %s", "HDMI1", val.c_str());
                        break;
                    case dsHDMI_IN_PORT_2:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI2.vrrEnable", val);
                        LOGINFO("Port %s: Persist VRR Support: %s", "HDMI2", val.c_str());
                        break;
                    case dsHDMI_IN_PORT_3:
                        device::HostPersistence::getInstance().persistHostProperty("HDMI3.vrrEnable", val);
                        LOGINFO("Port %s: Persist VRR Support: %s", "HDMI3", val.c_str());
                        break;
                    case dsHDMI_IN_PORT_NONE:
                    case dsHDMI_IN_PORT_4:
                    case dsHDMI_IN_PORT_MAX:
                        break;
                }
                retCode = WPEFramework::Core::ERROR_NONE;
            }
        }
        return retCode;
    }

    uint32_t GetVRRSupport(const HDMIInPort port, bool &vrrSupport) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        if (hdmiPort < dsHDMI_IN_PORT_MAX) {
            vrrSupport = m_vrrsupport[hdmiPort];
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    static dsError_t getVRRStatus (dsHdmiInPort_t iHdmiPort, dsHdmiInVrrStatus_t *vrrStatus) {
        dsError_t eRet = dsERR_GENERAL;
        typedef dsError_t (*dsHdmiInGetVRRStatus_t)(dsHdmiInPort_t iHdmiPort, dsHdmiInVrrStatus_t *vrrStatus);
        static dsHdmiInGetVRRStatus_t dsHdmiInGetVRRStatusFunc = 0;
        if (dsHdmiInGetVRRStatusFunc == 0) {
        void *dllib = dlopen(RDK_DSHAL_NAME, RTLD_LAZY);
        if (dllib) {
                dsHdmiInGetVRRStatusFunc = (dsHdmiInGetVRRStatus_t) dlsym(dllib, "dsHdmiInGetVRRStatus");
                if(dsHdmiInGetVRRStatusFunc == 0) {
                    LOGINFO("dsHdmiInGetVRRStatus (int) is not defined %s", dlerror());
                }
                else {
                    LOGINFO("dsHdmiInGetVRRStatusFunc loaded");
                }
                dlclose(dllib);
            }
            else {
                LOGINFO("dsHdmiInGetVRRStatus  Opening RDK_DSHAL_NAME [%s] failed %s",
                   RDK_DSHAL_NAME, dlerror());
            }
        }
        if (0 != dsHdmiInGetVRRStatusFunc) {
            eRet = dsHdmiInGetVRRStatusFunc (iHdmiPort, vrrStatus);
            LOGINFO("dsHdmiInGetVRRStatusFunc eRet: %d", eRet);
        }
        else {
            LOGINFO("%s:  dsHdmiInGetVRRStatusFunc = %p", __FUNCTION__, dsHdmiInGetVRRStatusFunc);
        }
        return eRet;
    }

    uint32_t GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) override
    {
        uint32_t retCode = WPEFramework::Core::ERROR_GENERAL;
        dsHdmiInPort_t hdmiPort = static_cast<dsHdmiInPort_t>(port);
        dsHdmiInVrrStatus_t status;
        if (getVRRStatus(hdmiPort, &status) == dsERR_NONE) {
            vrrStatus.vrrType = static_cast<HDMIInVRRType>(status.vrrType);
            vrrStatus.vrrFreeSyncFramerateHz = status.vrrAmdfreesyncFramerate_Hz;
            retCode = WPEFramework::Core::ERROR_NONE;
        }
        return retCode;
    }

    private:
};
