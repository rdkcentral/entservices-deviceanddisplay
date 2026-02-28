/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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

#include "DSController.h"

#include "UtilsLogging.h"
#include <syscall.h>
#include <unistd.h>
#include <fcntl.h>

extern "C" {
    #include "libIARM.h"
    #include "libIBusDaemon.h"
    #include "libIBus.h"
    #include "iarmUtil.h"
    #include "sysMgr.h"
    #include "dsMgr.h"
    #include "dsUtl.h"
    #include "dsError.h"
    #include "dsTypes.h"
    #include "dsRpc.h"
    #include "dsVideoPort.h"
    //#include "dsVideoResolutionSettings.h"
    #include "dsDisplay.h"
    //#include "dsAudioSettings.h"
    #include "dsAudio.h"
}

using namespace std;

namespace WPEFramework {
namespace Plugin {

//    SERVICE_REGISTRATION(DSController, 1, 0);

    DSController* DSController::_instance = nullptr;
    
    // Platform configuration constants
    #define RES_MAX_LEN 64
    #define RES_MAX_COUNT 6
    #define DEFAULT_PROGRESSIVE_FPS "60"
    #define RESOLUTION_BASE_UHD     "2160p"
    #define RESOLUTION_BASE_FHD     "1080p"
    #define RESOLUTION_BASE_FHD_INT "1080i"
    #define RESOLUTION_BASE_HD      "720p"
    #define RESOLUTION_BASE_PAL     "576p"
    #define RESOLUTION_BASE_NTSC    "480p"
    #define EU_PROGRESSIVE_FPS  "50"
    #define EU_INTERLACED_FPS   "25"
    
    // Static member variables
    static bool IsEUPlatform = false;
    static char fallBackResolutionList[RES_MAX_COUNT][RES_MAX_LEN];
    
    // Forward declarations of helper functions
    static bool isEUPlatform();
    static void setupPlatformConfig();
    static bool getSecondaryResolution(char* res, char *secRes);
    static void parseResolution(const char* pResn, char* bResn);
    static void getFallBackResolution(char* Resn, char *fbResn, int flag);
    static bool isResolutionSupported(dsDisplayEDID_t *edidData, int numResolutions, 
                                     int pNumResolutions, char *Resn, int* index);
    
    // EU Platform Detection - Migrated from old dsMgr.c
    static bool isEUPlatform()
    {
        ENTRY_LOG;
        char line[256];
        bool isEUflag = false;
        const char* devPropPath = "/etc/device.properties";
        char deviceProp[15] = "FRIENDLY_ID";
        const char* USRegion = " US";
        
        FILE *file = fopen(devPropPath, "r");
        if (file == NULL) {
            LOGERR("Unable to open file %s", devPropPath);
            EXIT_LOG;
            return false;
        }
        
        while (fgets(line, sizeof(line), file)) {
            if (strstr(line, deviceProp) != NULL) {
                if (strstr(line, USRegion) != NULL) {
                    LOGINFO("Detected US region: %s, isEUflag:%d", line, isEUflag);
                } else { // EU - UK/IT/DE
                    isEUflag = true;
                    LOGINFO("Detected EU region: %s, isEUflag:%d", line, isEUflag);
                }
                break;
            }
        }
        fclose(file);
        EXIT_LOG;
        return isEUflag;
    }
    
    // Setup Platform Configuration - Migrated from old dsMgr.c
    static void setupPlatformConfig()
    {
        ENTRY_LOG;
        const char* resList[] = {"2160p","1080p","1080i","720p","576p","480p"};
        int count = 0, n = sizeof(resList) / sizeof(resList[0]);
        
        IsEUPlatform = isEUPlatform();
        
        for (int i = 0; i < n; i++) {
            // Include 576p for EU only
            if ((strstr(resList[i], "576p") != NULL) && !IsEUPlatform) {
                continue;
            }
            if (count < RES_MAX_COUNT) {
                snprintf(fallBackResolutionList[count], RES_MAX_LEN, "%s", resList[i]);
                LOGINFO("Fallback resolution[%d]: %s", count, fallBackResolutionList[count]);
                count++;
            } else {
                break;
            }
        }
        EXIT_LOG;
    }
    
    // Get Secondary Resolution - Migrated from old dsMgr.c
    static bool getSecondaryResolution(char* res, char *secRes)
    {
        ENTRY_LOG;
        bool ret = true;
        
        if (strstr(res, RESOLUTION_BASE_HD) != NULL) {
            snprintf(secRes, RES_MAX_LEN, "%s", RESOLUTION_BASE_HD); // 720p
        } else if (strstr(res, RESOLUTION_BASE_FHD) != NULL) {
            snprintf(secRes, RES_MAX_LEN, "%s%s", RESOLUTION_BASE_FHD, DEFAULT_PROGRESSIVE_FPS); // 1080p60
        } else if (strstr(res, RESOLUTION_BASE_FHD_INT) != NULL) {
            snprintf(secRes, RES_MAX_LEN, "%s", RESOLUTION_BASE_FHD_INT); // 1080i
        } else if (strstr(res, RESOLUTION_BASE_UHD) != NULL) {
            snprintf(secRes, RES_MAX_LEN, "%s%s", RESOLUTION_BASE_UHD, DEFAULT_PROGRESSIVE_FPS); // 2160p60
        } else {
            ret = false; // For other resolutions 480p 576p
        }
        
        LOGINFO("Secondary resolution for %s: %s (ret=%d)", res, secRes, ret);
        EXIT_LOG;
        return ret;
    }
    
    // Parse Resolution - Migrated from old dsMgr.c
    static void parseResolution(const char* pResn, char* bResn)
    {
        ENTRY_LOG;
        char tmpResn[RES_MAX_LEN];
        int len = 0;
        
        snprintf(tmpResn, sizeof(tmpResn), "%s", pResn);
        char *token = strtok(tmpResn, "ip");
        strncpy(bResn, token, RES_MAX_LEN);
        len = strlen(bResn);
        
        if (strchr(pResn, 'i') != NULL) {
            snprintf(bResn + len, RES_MAX_LEN - len, "%s", "i");  // Append 'i'
        } else if (strchr(pResn, 'p') != NULL) {
            snprintf(bResn + len, RES_MAX_LEN - len, "%s", "p");  // Append 'p'
        }
        
        LOGINFO("Parsed resolution from %s to %s", pResn, bResn);
        EXIT_LOG;
    }
    
    // Get Fallback Resolution - Migrated from old dsMgr.c
    static void getFallBackResolution(char* Resn, char *fbResn, int flag)
    {
        ENTRY_LOG;
        char tmpResn[RES_MAX_LEN];
        snprintf(tmpResn, RES_MAX_LEN, "%s", Resn);
        int len = strlen(tmpResn);
        
        if (flag) { // EU
            if ((strcmp(Resn, RESOLUTION_BASE_UHD) == 0) || 
                (strcmp(Resn, RESOLUTION_BASE_FHD) == 0) || 
                (strcmp(Resn, RESOLUTION_BASE_HD) == 0)) {
                snprintf(tmpResn + len, sizeof(tmpResn) - len, "%s", EU_PROGRESSIVE_FPS);  // 2160p50, 1080p50, 720p50
            } else if (strcmp(Resn, RESOLUTION_BASE_FHD_INT) == 0) {
                snprintf(tmpResn + len, sizeof(tmpResn) - len, "%s", EU_INTERLACED_FPS);  // 1080i25
            } else {
                // do nothing for 576p, 480p
            }
        } else { // US
            if ((strcmp(Resn, RESOLUTION_BASE_UHD) == 0) || 
                (strcmp(Resn, RESOLUTION_BASE_FHD) == 0)) {
                snprintf(tmpResn + len, sizeof(tmpResn) - len, "%s", DEFAULT_PROGRESSIVE_FPS);  // 2160p60, 1080p60
            }
        }
        
        snprintf(fbResn, RES_MAX_LEN, "%s", tmpResn);
        LOGINFO("Fallback resolution for %s (EU=%d): %s", Resn, flag, fbResn);
        EXIT_LOG;
    }
    
    // Check if Resolution is Supported - Migrated from old dsMgr.c
    static bool isResolutionSupported(dsDisplayEDID_t *edidData, int numResolutions, 
                                     int pNumResolutions, char *Resn, int* index)
    {
        ENTRY_LOG;
        bool supported = false;
        dsVideoPortResolution_t *setResn = NULL;
        
        for (int i = numResolutions - 1; i >= 0; i--) {
            setResn = &(edidData->suppResolutionList[i]);
            if (strcmp(setResn->name, Resn) == 0) {
                // Check if platform supports this resolution
                // Note: kResolutions would need to be defined or passed as parameter
                // For now, we'll mark as supported if found in EDID
                LOGINFO("Resolution supported in EDID: %s", Resn);
                supported = true;
                *index = i;
                break;
            }
        }
        
        EXIT_LOG;
        return supported;
    }

    DSController::DSController()
        : _resolutionThreadID(0)
        , _hotplugEventSrc(0)
        , _tuneReady(0)
        , _initResolutionFlag(0)
        , _resolutionRetryCount(5)
        , _hdcpAuthenticated(false)
        , _ignoreEdid(false)
        , _displayEventStatus(dsDISPLAY_EVENT_MAX)
        , _easMode(0) // IARM_BUS_SYS_MODE_NORMAL
    {
        ENTRY_LOG;
        DSController::_instance = this;
        LOGINFO("DSController Constructor - Instance Address: %p", this);
        
        // Initialize pthread mutex and condition variable
        pthread_mutex_init(&_mutexLock, NULL);
        pthread_cond_init(&_mutexCond, NULL);
        
        // Setup platform configuration (EU/US detection and fallback resolution list)
        setupPlatformConfig();
        
        Start();
        EXIT_LOG;
    }

    DSController::~DSController() {
        ENTRY_LOG;
        LOGINFO("DSController Destructor - Instance Address: %p", this);
        
        // Cleanup pthread resources
        pthread_mutex_destroy(&_mutexLock);
        pthread_cond_destroy(&_mutexCond);
        
        EXIT_LOG;
    }

    DSController* DSController::instance(DSController* controller)
    {
        ENTRY_LOG;
        if (controller != nullptr) {
            _instance = controller;
        }
        EXIT_LOG;
        return _instance;
    }

    // Migrated from DSMgr_Start
    uint32_t DSController::Start()
    {
        ENTRY_LOG;
        
        setvbuf(stdout, NULL, _IOLBF, 0);
        LOGINFO("DSController::Start - Entering");
        
        // Register with IARM Libs and Connect
        IARM_Bus_Init(IARM_BUS_DSMGR_NAME);
        IARM_Bus_Connect();
        IARM_Bus_RegisterEvent(IARM_BUS_DSMGR_EVENT_MAX);
        
        // Initialize the DS Manager - DS Srv and DS HAL
        Init();
        
        _initResolutionFlag = 1;
        
        // Get Ignore EDID status
        dsEdidIgnoreParam_t ignoreEdidParam;
        memset(&ignoreEdidParam, 0, sizeof(ignoreEdidParam));
        ignoreEdidParam.handle = dsVIDEOPORT_TYPE_HDMI;
        // _dsGetIgnoreEDIDStatus(&ignoreEdidParam);
        _ignoreEdid = ignoreEdidParam.ignoreEDID;
        LOGINFO("ResOverride DSController::Start _ignoreEdid: %d", _ignoreEdid);
        
        // Register the Events
        // IARM_Bus_RegisterEventHandler(IARM_BUS_SYSMGR_NAME, IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE, _EventHandler);
        // IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG, _EventHandler);
        // IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDCP_STATUS, _EventHandler);
        
        // Register EAS handler
        // IARM_Bus_RegisterCall(IARM_BUS_COMMON_API_SysModeChange, _SysModeChange);
        
        // Initialize power event listener (refactored dsMGR code)
        // initPwrEventListner();
        
        // Create Thread for listening Hot Plug events
        InitializeResolutionThread();
        
        // Read the HDMI DDC Line delay
        FILE* fDSCtrptr = fopen("/opt/ddcDelay", "r");
        if (NULL != fDSCtrptr) {
            if (0 > fscanf(fDSCtrptr, "%d", &_resolutionRetryCount)) {
                LOGERR("Error: fscanf on ddcDelay failed");
            }
            fclose(fDSCtrptr);
        }
        LOGINFO("Retry DS manager Resolution count is %d", _resolutionRetryCount);
        
        // Get Tune Ready status on startup
        IARM_Bus_SYSMgr_GetSystemStates_Param_t tuneReadyParam;
        IARM_Bus_Call(IARM_BUS_SYSMGR_NAME, IARM_BUS_SYSMGR_API_GetSystemStates, 
                     &tuneReadyParam, sizeof(tuneReadyParam));
        LOGINFO("Tune Ready Status on start up is %d", tuneReadyParam.TuneReadyStatus.state);
        
        if (1 == tuneReadyParam.TuneReadyStatus.state) {
            _tuneReady = 1;
        }
        
        if (!IsHDMIConnected()) {
            LOGERR("HDMI not connected at bootup - Schedule a handler to set the resolution");
            SetVideoPortResolution();
        }
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    // Migrated from DSMgr_Stop
    uint32_t DSController::Stop()
    {
        ENTRY_LOG;
        
        Deinit();
        
        IARM_Bus_Disconnect();
        IARM_Bus_Term();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    // Migrated from dsMgr_init
    void DSController::Init()
    {
        ENTRY_LOG;
        
        LOGINFO("DSController::Init - Initializing Device Settings subsystems");
        
        // Initialize device settings subsystems
        // These would call the respective *Mgr_init functions
        // dsHostInit();
        // dsDisplayMgr_init();
        // dsAudioMgr_init();
        // dsVideoPortMgr_init();
        // dsVideoDeviceMgr_init();
        // dsFPDMgr_init();
        // dsHostMgr_init();
        // dsHdmiInMgr_init();
        // dsCompositeInMgr_init();
        
        EXIT_LOG;
    }

    // Migrated from dsMgr_term
    void DSController::Deinit()
    {
        ENTRY_LOG;
        
        LOGINFO("DSController::Deinit - Terminating Device Settings subsystems");
        
        // Terminate device settings subsystems
        // dsAudioMgr_term();
        // dsVideoPortMgr_term();
        // dsVideoDeviceMgr_term();
        // dsFPDMgr_term();
        // dsDisplayMgr_term();
        // dsHostMgr_term();
        // dsHdmiInMgr_term();
        // dsCompositeInMgr_term();
        
        EXIT_LOG;
    }

    void DSController::InitializeResolutionThread()
    {
        ENTRY_LOG;
        pthread_create(&_resolutionThreadID, NULL, ResolutionThreadFunc, this);
        EXIT_LOG;
    }

    // Static helper methods
    intptr_t DSController::GetVideoPortHandle(dsVideoPortType_t port)
    {
        ENTRY_LOG;
        dsVideoPortGetHandleParam_t vidPortParam;
        memset(&vidPortParam, 0, sizeof(vidPortParam));
        vidPortParam.type = port;
        vidPortParam.index = 0;
        // Call the RPC function to get video port handle
        // _dsGetVideoPort(&vidPortParam);
        EXIT_LOG;
        return vidPortParam.handle;
    }

    bool DSController::IsHDMIConnected()
    {
        ENTRY_LOG;
        dsVideoPortIsDisplayConnectedParam_t ConParam;
        memset(&ConParam, 0, sizeof(ConParam));
        ConParam.handle = GetVideoPortHandle(dsVIDEOPORT_TYPE_HDMI);
        // Call the RPC function to check if display is connected
        // _dsIsDisplayConnected(&ConParam);
        EXIT_LOG;
        return ConParam.connected;
    }

    void* DSController::ResolutionThreadFunc(void *arg)
    {
        ENTRY_LOG;
        DSController* controller = static_cast<DSController*>(arg);
        
        if (controller) {
            // Loop forever waiting for events
            while (1) {
                LOGINFO("ResolutionThreadFunc... wait for for HDMI or Tune Ready Events");
                
                // Wait for the Event
                pthread_mutex_lock(&controller->_mutexLock);
                pthread_cond_wait(&controller->_mutexCond, &controller->_mutexLock);
                pthread_mutex_unlock(&controller->_mutexLock);
                
                LOGINFO("Setting Resolution On:: HDMI %s Event with TuneReady status = %d",
                       (controller->_displayEventStatus == dsDISPLAY_EVENT_CONNECTED ? "Connect" : "Disconnect"),
                       controller->_tuneReady);
                
                // On hot plug event, clear event source
                if (controller->_hotplugEventSrc) {
                    LOGINFO("Cleared Hot Plug Event Time source %d", controller->_hotplugEventSrc);
                    controller->_hotplugEventSrc = 0;
                }
                
                // Set the Resolution only on HDMI Hot plug Connect and Tune Ready events
                if ((1 == controller->_tuneReady) && (dsDISPLAY_EVENT_CONNECTED == controller->_displayEventStatus)) {
                    // Set Video Output Port Resolution
                    if (controller->_hdcpAuthenticated) {
                        controller->SetVideoPortResolution();
                    }
                    // Set audio mode on HDMI hot plug
                    controller->SetAudioMode();
                }
                // Set the Resolution only on HDMI Hot plug - Disconnect and Tune Ready event
                else if ((1 == controller->_tuneReady) && (dsDISPLAY_EVENT_DISCONNECTED == controller->_displayEventStatus)) {
                    controller->_hdcpAuthenticated = false;
                    // Check if component port is present
                    // if (isComponentPortPresent())
                    {
                        // Schedule resolution handler after 5 seconds using WPEFramework
                        Core::IWorkerPool::Instance().Schedule(
                            Core::Time::Now().Add(5000),
                            LambdaJob::Create(
                                controller,
                                [controller]() {
                                    LOGINFO("Set Video Resolution after delayed time");
                                    controller->SetVideoPortResolution();
                                    controller->_hotplugEventSrc = 0;
                                }
                            )
                        );
                        LOGINFO("Schedule a handler to set the resolution after 5 sec");
                    }
                }
            }
        }
        
        EXIT_LOG;
        return nullptr;
    }

    void DSController::SetVideoPortResolution()
    {
        ENTRY_LOG;
        LOGINFO("SetVideoPortResolution - Enter");
        
        intptr_t hdmiHandle = 0;
        intptr_t compHandle = 0;
        bool connected = false;
        
        hdmiHandle = GetVideoPortHandle(dsVIDEOPORT_TYPE_HDMI);
        if (hdmiHandle != 0) {
            usleep(100 * 1000); // wait for 100 milliseconds
            
            // Check for HDMI DDC Line when HDMI is connected
            connected = IsHDMIConnected();
            if (_initResolutionFlag && connected) {
                #ifdef _INIT_RESN_SETTINGS
                // Wait for _resolutionRetryCount
                int iCount = 0;
                while (iCount < _resolutionRetryCount) {
                    sleep(1); // wait for 1 sec
                    // if (dsGetHDMIDDCLineStatus()) {
                    //     break;
                    // }
                    LOGINFO("Waiting for HDMI DDC Line to be ready for resolution Change...");
                    iCount++;
                }
                #endif
            }
            
            // Set HDMI Resolution if Connected else Component or Composite Resolution
            if (connected) {
                LOGINFO("Setting HDMI resolution..........");
                SetResolution(&hdmiHandle, dsVIDEOPORT_TYPE_HDMI);
            } else {
                compHandle = GetVideoPortHandle(dsVIDEOPORT_TYPE_COMPONENT);
                
                if (0 != compHandle) {
                    LOGINFO("Setting Component/Composite Resolution..........");
                    SetResolution(&compHandle, dsVIDEOPORT_TYPE_COMPONENT);
                } else {
                    LOGINFO("NULL Handle for component");
                    
                    intptr_t compositeHandle = GetVideoPortHandle(dsVIDEOPORT_TYPE_BB);
                    if (0 != compositeHandle) {
                        LOGINFO("Setting BB Composite Resolution..........");
                        SetResolution(&compositeHandle, dsVIDEOPORT_TYPE_BB);
                    } else {
                        LOGINFO("NULL Handle for Composite");
                        intptr_t rfHandle = GetVideoPortHandle(dsVIDEOPORT_TYPE_RF);
                        if (0 != rfHandle) {
                            LOGINFO("Setting RF Resolution..........");
                            SetResolution(&rfHandle, dsVIDEOPORT_TYPE_RF);
                        } else {
                            LOGINFO("NULL Handle for RF");
                        }
                    }
                }
            }
        } else {
            LOGINFO("NULL Handle for HDMI");
        }
        
        LOGINFO("SetVideoPortResolution - Exit");
        EXIT_LOG;
    }

    void DSController::SetResolution(intptr_t* handle, dsVideoPortType_t portType)
    {
        ENTRY_LOG;
        
        intptr_t handleValue = *handle;
        intptr_t displayHandle = 0;
        int numResolutions = 0;
        int resIndex = 0;
        bool isValidResolution = false;
        dsVideoPortSetResolutionParam_t setParam;
        dsVideoPortGetResolutionParam_t getParam;
        dsVideoPortResolution_t* setResn = NULL;
        dsDisplayEDID_t edidData;
        dsDisplayGetEDIDParam_t edidParam;
        
        // Default Resolution Compatible check is false - Do not Force compatible resolution on startup
        setParam.forceCompatible = false;
        
        // Initialize the struct
        memset(&edidData, 0, sizeof(edidData));
        
        // Return if Handle is NULL
        if (handleValue == 0) {
            LOGERR("SetResolution - Got NULL Handle");
            EXIT_LOG;
            return;
        }
        
        // Get the User Persisted Resolution Based on Handle
        memset(&getParam, 0, sizeof(getParam));
        getParam.handle = handleValue;
        getParam.toPersist = true;
        // _dsGetResolution(&getParam);
        dsVideoPortResolution_t* presolution = &getParam.resolution;
        LOGINFO("Got User Persisted Resolution - %s", presolution->name);
        
        if (portType == dsVIDEOPORT_TYPE_HDMI) {
            // Get The Display Handle
            dsGetDisplay(dsVIDEOPORT_TYPE_HDMI, 0, &displayHandle);
            if (displayHandle) {
                // Get the EDID Display Handle
                memset(&edidParam, 0, sizeof(edidParam));
                edidParam.handle = displayHandle;
                // _dsGetEDID(&edidParam);
                memcpy(&edidData, &edidParam.edid, sizeof(edidParam.edid));
                DumpHdmiEdidInfo(&edidData);
                numResolutions = edidData.numOfSupportedResolution;
                LOGINFO("numResolutions is %d", numResolutions);
                
                // If HDMI is connected and Low power Mode, TV might not transmit EDID information
                // Change the Resolution in Next Hot plug. Do not set if TV is in DVI mode
                if ((0 == numResolutions) || (!(edidData.hdmiDeviceType))) {
                    LOGERR("Do not Set Resolution..The HDMI is not Ready !!");
                    LOGERR("numResolutions = %d edidData.hdmiDeviceType = %d !!", numResolutions, edidData.hdmiDeviceType);
                    EXIT_LOG;
                    return;
                }
                
                // Check if Persisted Resolution matches with TV Resolution list
                for (int i = 0; i < numResolutions; i++) {
                    setResn = &(edidData.suppResolutionList[i]);
                    LOGINFO("presolution->name : %s, resolution->name : %s", presolution->name, setResn->name);
                    if ((strcmp(presolution->name, setResn->name) == 0)) {
                        LOGINFO("Breaking..Got Platform Resolution - %s", setResn->name);
                        isValidResolution = true;
                        setParam.forceCompatible = true;
                        break;
                    }
                }
                
                // SECONDARY VIC Settings only for EU platforms
                // Check if alternate freq or secondary resolution supported by the TV
                // if resolution with 50Hz not supported check for same resolution with 60Hz
                // Other FPS like 30, 25, 24 not used to avoid any judders
                if (false == isValidResolution && IsEUPlatform) {
                    char secResn[RES_MAX_LEN];
                    // get secondary resolution based on presolution
                    if (getSecondaryResolution(presolution->name, secResn)) {
                        int pNumResolutions = 0; // Would need actual platform resolution count
                        if (isResolutionSupported(&edidData, numResolutions, pNumResolutions, secResn, &resIndex)) {
                            setResn = &(edidData.suppResolutionList[resIndex]);
                            LOGINFO("Breaking..Got Secondary Resolution - %s", setResn->name);
                            isValidResolution = true;
                            setParam.forceCompatible = true;
                        }
                    }
                }
                
                // Fallback to next best resolution
                if (false == isValidResolution) {
                    int index = 0;
                    char baseResn[RES_MAX_LEN], fbResn[RES_MAX_LEN];
                    parseResolution(presolution->name, baseResn);
                    int fNumResolutions = sizeof(fallBackResolutionList) / sizeof(fallBackResolutionList[0]);
                    
                    // Find current resolution in fallback list
                    for (int i = 0; i < fNumResolutions; i++) {
                        if (strcmp(fallBackResolutionList[i], baseResn) == 0) {
                            index = i;
                            break;
                        }
                    }
                    
                    // Try fallback resolutions in order
                    for (int i = index + 1; i < fNumResolutions; i++) {
                        int pNumResolutions = 0; // Would need actual platform resolution count
                        
                        if (IsEUPlatform) {
                            getFallBackResolution(fallBackResolutionList[i], fbResn, 1); // EU fps
                            LOGINFO("Check next resolution: %s", fbResn);
                            if (isResolutionSupported(&edidData, numResolutions, pNumResolutions, fbResn, &resIndex)) {
                                isValidResolution = true;
                            }
                        }
                        
                        if (!isValidResolution) {
                            getFallBackResolution(fallBackResolutionList[i], fbResn, 0); // default fps
                            LOGINFO("Check next resolution: %s", fbResn);
                            if (isResolutionSupported(&edidData, numResolutions, pNumResolutions, fbResn, &resIndex)) {
                                isValidResolution = true;
                            }
                        }
                        
                        if (isValidResolution) {
                            setResn = &(edidData.suppResolutionList[resIndex]);
                            LOGINFO("Got Next Best Resolution - %s", setResn->name);
                            break;
                        }
                    }
                }
                
                // The Persisted Resolution Does not match with TV and Platform Resolution List
                // Force Platform Default Resolution (720p)
                if (false == isValidResolution) {
                    LOGINFO("Trying default platform resolution (720p)");
                    for (int i = 0; i < numResolutions; i++) {
                        setResn = &(edidData.suppResolutionList[i]);
                        if ((strcmp("720p", setResn->name) == 0)) {
                            isValidResolution = true;
                            LOGINFO("Breaking..Got Default Platform Resolution - %s", setResn->name);
                            break;
                        }
                    }
                }
                
                // Take 480p as resolution if both above cases fail
                if (false == isValidResolution) {
                    LOGINFO("Trying 480p as last resort");
                    for (int i = 0; i < numResolutions; i++) {
                        setResn = &(edidData.suppResolutionList[i]);
                        if ((strcmp("480p", setResn->name) == 0)) {
                            LOGINFO("Breaking..Default to 480p Resolution - %s", setResn->name);
                            isValidResolution = true;
                            break;
                        }
                    }
                }
                
                // Boot with any Resolution Supported by both TV and Platform
                if (false == isValidResolution) {
                    LOGINFO("Using any TV supported resolution as final fallback");
                    for (int i = 0; i < numResolutions; i++) {
                        setResn = &(edidData.suppResolutionList[i]);
                        LOGINFO("Boot with TV Supported Resolution %s", setResn->name);
                        isValidResolution = true;
                        break;
                    }
                }
            }
        } else if (portType == dsVIDEOPORT_TYPE_COMPONENT || portType == dsVIDEOPORT_TYPE_BB || portType == dsVIDEOPORT_TYPE_RF) {
            // Set the Component / Composite Resolution
            LOGINFO("Setting resolution for non-HDMI port type: %d", portType);
            // numResolutions = dsUTL_DIM(kResolutions);
            // For component/composite, check persisted resolution against platform resolutions
            // If not valid, default to 720p or platform default
            // This logic would require kResolutions array to be available
        }
        
        // If the Persisted Resolution settings does not match with Platform Resolution
        // Force Default on Component/Composite (720p or kDefaultResIndex)
        if (false == isValidResolution && setResn == NULL) {
            LOGINFO("No valid resolution found, using hardcoded default");
            // setResn = &kResolutions[kDefaultResIndex]; // Would need kResolutions array
        }
        
        // Set The Video Port Resolution in Requested Handle
        if (setResn != NULL) {
            setParam.handle = handleValue;
            setParam.toPersist = false;
            
            // Check if 4K support is disabled and last known resolution is 4K, default to 720p
            dsForceDisable4KParam_t res_4K_override;
            memset(&res_4K_override, 0, sizeof(res_4K_override));
            // _dsGetForceDisable4K((void*)&res_4K_override);
            if (true == res_4K_override.disable) {
                if (0 == strncmp(presolution->name, "2160", 4)) {
                    LOGINFO("User persisted 4K resolution. Now limiting to default (720p) as 4K support is disabled");
                    // setResn = &kResolutions[kDefaultResIndex]; // Would need kResolutions array
                }
            }
            
            setParam.resolution = *setResn;
            
            #ifdef _INIT_RESN_SETTINGS
            if (0 == _initResolutionFlag) {
                LOGINFO("Init Platform Resolution - %s", setResn->name);
                // _dsInitResolution(&setParam);
                EXIT_LOG;
                return;
            }
            #endif
            
            LOGINFO("Setting resolution to: %s", setResn->name);
            // _dsSetResolution(&setParam);
        } else {
            LOGERR("Failed to find any valid resolution!");
        }
        
        EXIT_LOG;
    }

    void DSController::SetAudioMode()
    {
        ENTRY_LOG;
        
        if (_easMode == 1) { // IARM_BUS_SYS_MODE_EAS
            LOGINFO("EAS In progress..Do not Modify Audio");
            return;
        }
        
        dsAudioGetHandleParam_t getHandle;
        dsAudioSetStereoModeParam_t setMode;
        // int numPorts = dsUTL_DIM(kSupportedPortTypes);
        int numPorts = 0; // Placeholder
        
        for (int i = 0; i < numPorts; i++) {
            // const dsAudioPortType_t* audioPort = &kSupportedPortTypes[i];
            memset(&getHandle, 0, sizeof(getHandle));
            // getHandle.type = *audioPort;
            getHandle.index = 0;
            // _dsGetAudioPort(&getHandle);
            
            memset(&setMode, 0, sizeof(setMode));
            setMode.handle = getHandle.handle;
            setMode.toPersist = true;
            // _dsGetStereoMode(&setMode);
            
            if (getHandle.type == dsAUDIOPORT_TYPE_HDMI) {
                // Check if it is connected
                intptr_t vHandle = 0;
                int autoMode = 0;
                bool connected = false;
                bool isSurround = false;
                
                dsVideoPortGetHandleParam_t param;
                memset(&param, 0, sizeof(param));
                param.type = dsVIDEOPORT_TYPE_HDMI;
                param.index = 0;
                // _dsGetVideoPort(&param);
                vHandle = param.handle;
                
                dsVideoPortIsDisplayConnectedParam_t connParam;
                memset(&connParam, 0, sizeof(connParam));
                connParam.handle = vHandle;
                // _dsIsDisplayConnected(&connParam);
                connected = connParam.connected;
                
                if (!connected) {
                    LOGINFO("HDMI Not Connected ..Do not Set Audio on HDMI !!!");
                    continue;
                }
                
                dsAudioSetStereoAutoParam_t autoParam;
                memset(&autoParam, 0, sizeof(autoParam));
                autoParam.handle = getHandle.handle;
                // _dsGetStereoAuto(&autoParam);
                autoMode = autoParam.autoMode;
                
                if (autoMode) {
                    // If auto, then force surround
                    setMode.mode = dsAUDIO_STEREO_SURROUND;
                }
                
                // Assume surround is supported
                isSurround = true;
                
                if (!isSurround) {
                    // If Surround not supported, then force Stereo
                    setMode.mode = dsAUDIO_STEREO_STEREO;
                    LOGINFO("Surround mode not Supported on HDMI ..Set Stereo");
                }
            }
            
            LOGINFO("Audio mode for audio port %d is : %d", getHandle.type, setMode.mode);
            setMode.toPersist = false;
            // _dsSetStereoMode(&setMode);
        }
        
        EXIT_LOG;
    }

    void DSController::SetEASAudioMode()
    {
        ENTRY_LOG;
        
        if (_easMode != 1) { // IARM_BUS_SYS_MODE_EAS
            LOGINFO("EAS Not In progress..Do not Modify Audio");
            return;
        }
        
        dsAudioGetHandleParam_t getHandle;
        dsAudioSetStereoModeParam_t setMode;
        // int numPorts = dsUTL_DIM(kSupportedPortTypes);
        int numPorts = 0; // Placeholder
        
        for (int i = 0; i < numPorts; i++) {
            // const dsAudioPortType_t* audioPort = &kSupportedPortTypes[i];
            memset(&getHandle, 0, sizeof(getHandle));
            // getHandle.type = *audioPort;
            getHandle.index = 0;
            // _dsGetAudioPort(&getHandle);
            
            memset(&setMode, 0, sizeof(setMode));
            setMode.handle = getHandle.handle;
            setMode.toPersist = false;
            // _dsGetStereoMode(&setMode);
            
            if (setMode.mode == dsAUDIO_STEREO_PASSTHRU) {
                // In EAS, fallback to Stereo
                setMode.mode = dsAUDIO_STEREO_STEREO;
            }
            
            LOGINFO("EAS Audio mode for audio port %d is : %d", getHandle.type, setMode.mode);
            setMode.toPersist = false;
            // _dsSetStereoMode(&setMode);
        }
        
        EXIT_LOG;
    }

    void DSController::SetBackgroundColor(dsVideoBackgroundColor_t color)
    {
        ENTRY_LOG;
        
        // Get the HDMI Video Port Parameter
        dsVideoPortGetHandleParam_t vidPortParam;
        memset(&vidPortParam, 0, sizeof(vidPortParam));
        vidPortParam.type = dsVIDEOPORT_TYPE_HDMI;
        vidPortParam.index = 0;
        // _dsGetVideoPort(&vidPortParam);
        
        if (vidPortParam.handle != 0) {
            dsSetBackgroundColorParam_t setBGColorParam;
            memset(&setBGColorParam, 0, sizeof(setBGColorParam));
            setBGColorParam.color = color;
            setBGColorParam.handle = vidPortParam.handle;
            // _dsSetBackgroundColor(&setBGColorParam);
        }
        
        EXIT_LOG;
    }

    void DSController::DumpHdmiEdidInfo(dsDisplayEDID_t* pedidData)
    {
        ENTRY_LOG;
        LOGINFO("Connected HDMI Display Device Info");
        
        if (nullptr == pedidData) {
            LOGINFO("Received EDID is NULL");
            return;
        }
        
        if (pedidData->monitorName)
            LOGINFO("HDMI Monitor Name is %s", pedidData->monitorName);
        LOGINFO("HDMI Manufacturing ID is %d", pedidData->serialNumber);
        LOGINFO("HDMI Product Code is %d", pedidData->productCode);
        LOGINFO("HDMI Device Type is %s", pedidData->hdmiDeviceType ? "HDMI" : "DVI");
        LOGINFO("HDMI Sink Device %s a Repeater", pedidData->isRepeater ? "is" : "is not");
        LOGINFO("HDMI Physical Address is %d:%d:%d:%d",
                pedidData->physicalAddressA, pedidData->physicalAddressB,
                pedidData->physicalAddressC, pedidData->physicalAddressD);
        
        EXIT_LOG;
    }

    void DSController::ScheduleEdidDump()
    {
        ENTRY_LOG;
        // Schedule EDID dump after 1 second using WPEFramework
        Core::IWorkerPool::Instance().Schedule(
            Core::Time::Now().Add(1000),
            LambdaJob::Create(
                this,
                [this]() {
                    LOGINFO("dumpEdidOnChecksumDiff HDMI-EDID Dump>>>>>>>>>>>>>>");
                    intptr_t displayHandle = 0;
                    dsGetDisplay(dsVIDEOPORT_TYPE_HDMI, 0, &displayHandle);
                    if (displayHandle) {
                        int length = 0;
                        dsDisplayGetEDIDBytesParam_t EdidBytesParam;
                        static int cached_EDID_checksum = 0;
                        int current_EDID_checksum = 0;
                        memset(&EdidBytesParam, 0, sizeof(EdidBytesParam));
                        EdidBytesParam.handle = displayHandle;
                        length = EdidBytesParam.length;
                        if ((length > 0) && (length <= 512)) {
                            unsigned char* edidBytes = EdidBytesParam.bytes;
                            for (int i = 0; i < (length / 128); i++)
                                current_EDID_checksum += edidBytes[(i+1)*128 - 1];
                            if ((cached_EDID_checksum == 0) || (current_EDID_checksum != cached_EDID_checksum)) {
                                cached_EDID_checksum = current_EDID_checksum;
                                LOGINFO("HDMI-EDID Dump detected changes");
                            }
                        }
                    }
                }
            )
        );
        EXIT_LOG;
    }

    void DSController::EventHandler(const char *owner, int eventId, void *data, size_t len)
    {
        ENTRY_LOG;
        
        // Allows dsmgr to set initial resolution irrespective of ignore edid only during boot
        static bool bootup_flag_enabled = true;
        
        // Handle only Sys Manager Events
        if (strcmp(owner, IARM_BUS_SYSMGR_NAME) == 0) {
            // Only handle state events
            if (eventId != IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE) return;
            
            IARM_Bus_SYSMgr_EventData_t* sysEventData = (IARM_Bus_SYSMgr_EventData_t*)data;
            IARM_Bus_SYSMgr_SystemState_t stateId = sysEventData->data.systemStates.stateId;
            int state = sysEventData->data.systemStates.state;
            LOGINFO("EventHandler invoked for stateid %d of state %d", stateId, state);
            
            switch (stateId) {
                case IARM_BUS_SYSMGR_SYSSTATE_TUNEREADY:
                    LOGINFO("Tune Ready Events in DS Manager");
                    
                    if (0 == _tuneReady) {
                        _tuneReady = 1;
                        
                        // Set audio mode from persistent
                        SetAudioMode();
                        
                        // Un-block the Resolution Settings Thread
                        pthread_mutex_lock(&_mutexLock);
                        pthread_cond_signal(&_mutexCond);
                        pthread_mutex_unlock(&_mutexLock);
                    }
                    break;
                default:
                    break;
            }
        } else if (strcmp(owner, IARM_BUS_DSMGR_NAME) == 0) {
            switch (eventId) {
                case IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG:
                {
                    IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
                    
                    LOGINFO("Got HDMI %s Event",
                           (eventData->data.hdmi_hpd.event == dsDISPLAY_EVENT_CONNECTED ? "Connect" : "Disconnect"));
                    
                    SetBackgroundColor(dsVIDEO_BGCOLOR_NONE);
                    
                    // Un-Block the Resolution Settings Thread
                    pthread_mutex_lock(&_mutexLock);
                    _displayEventStatus = ((eventData->data.hdmi_hpd.event == dsDISPLAY_EVENT_CONNECTED) ?
                                          dsDISPLAY_EVENT_CONNECTED : dsDISPLAY_EVENT_DISCONNECTED);
                    pthread_cond_signal(&_mutexCond);
                    pthread_mutex_unlock(&_mutexLock);
                }
                break;
                
                case IARM_BUS_DSMGR_EVENT_HDCP_STATUS:
                {
                    IARM_Bus_DSMgr_EventData_t* eventData = (IARM_Bus_DSMgr_EventData_t*)data;
                    IARM_Bus_SYSMgr_EventData_t HDCPeventData;
                    int status = eventData->data.hdmi_hdcp.hdcpStatus;
                    
                    // HDCP is enabled
                    HDCPeventData.data.systemStates.stateId = IARM_BUS_SYSMGR_SYSSTATE_HDCP_ENABLED;
                    HDCPeventData.data.systemStates.state = 1;
                    
                    if (status == dsHDCP_STATUS_AUTHENTICATED) {
                        LOGINFO("Changed status to HDCP Authentication Pass !!!!!!!!");
                        HDCPeventData.data.systemStates.state = 1;
                        _hdcpAuthenticated = true;
                        LOGINFO("HDCP success - Cleared hotplug_event_src Time source %d and set resolution immediately", _hotplugEventSrc);
                        
                        if (_hotplugEventSrc) {
                            _hotplugEventSrc = 0;
                        }
                        
                        SetBackgroundColor(dsVIDEO_BGCOLOR_NONE);
                        if ((!_ignoreEdid) || bootup_flag_enabled) {
                            SetVideoPortResolution();
                            if (bootup_flag_enabled)
                                bootup_flag_enabled = false;
                        }
                        ScheduleEdidDump();
                    } else if (status == dsHDCP_STATUS_AUTHENTICATIONFAILURE) {
                        LOGERR("Changed status to HDCP Authentication Fail !!!!!!!!");
                        HDCPeventData.data.systemStates.state = 0;
                        SetBackgroundColor(dsVIDEO_BGCOLOR_BLUE);
                        _hdcpAuthenticated = false;
                        if (!_ignoreEdid) {
                            SetVideoPortResolution();
                        }
                        ScheduleEdidDump();
                    }
                    
                    IARM_Bus_BroadcastEvent(IARM_BUS_SYSMGR_NAME, (IARM_EventId_t)IARM_BUS_SYSMGR_EVENT_SYSTEMSTATE,
                                          (void*)&HDCPeventData, sizeof(HDCPeventData));
                }
                break;
                
                default:
                    break;
            }
        }
        
        EXIT_LOG;
    }

    void DSController::SysModeChange(void *arg)
    {
        ENTRY_LOG;
        
        IARM_Bus_CommonAPI_SysModeChange_Param_t* param = (IARM_Bus_CommonAPI_SysModeChange_Param_t*)arg;
        int isNextEAS = 0; // IARM_BUS_SYS_MODE_NORMAL
        
        LOGINFO("Recvd Sysmode Change::New mode --> %d, Old mode --> %d", param->newMode, param->oldMode);
        
        if ((param->newMode == IARM_BUS_SYS_MODE_EAS) ||
            (param->newMode == IARM_BUS_SYS_MODE_NORMAL)) {
            isNextEAS = param->newMode;
        } else {
            // Do not process any other mode change as of now for DS Manager
            return;
        }
        
        if ((_easMode == IARM_BUS_SYS_MODE_EAS) && (isNextEAS == IARM_BUS_SYS_MODE_NORMAL)) {
            _easMode = IARM_BUS_SYS_MODE_NORMAL;
            SetAudioMode();
        } else if ((_easMode == IARM_BUS_SYS_MODE_NORMAL) && (isNextEAS == IARM_BUS_SYS_MODE_EAS)) {
            // Change the Audio Mode to Stereo if Current Audio Setting is Passthrough
            _easMode = IARM_BUS_SYS_MODE_EAS;
            SetEASAudioMode();
        } else {
            // no op for no mode change
        }
        
        EXIT_LOG;
    }

} // namespace Plugin
} // namespace WPEFramework
