/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
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
**/

#include "Warehouse.h"

#include <algorithm>
#include <fstream>
#include "secure_wrapper.h"
#include <regex.h>
#include <regex>
#include <time.h>
#include <sstream>

#if defined(USE_IARMBUS) || defined(USE_IARM_BUS)
#include "UtilsIarm.h"
#include "sysMgr.h"
#include "pwrMgr.h"
#endif

#include "UtilsCStr.h"
#include "UtilsJsonRpc.h"
#include "UtilsString.h"
#include "UtilsfileExists.h"
#include "UtilsgetFileContent.h"

#include "frontpanel.h"

#include "rfcapi.h"

#define WAREHOUSE_RFC_CALLERID                  "Warehouse"
#define WAREHOUSE_HOSTCLIENT_NAME1_RFC_PARAM    "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.CommonProperties.WarehouseHost.CName1"
#define WAREHOUSE_HOSTCLIENT_NAME2_RFC_PARAM    "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.CommonProperties.WarehouseHost.CName2"
#define WAREHOUSE_HOSTCLIENT_TAIL_RFC_PARAM     "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.CommonProperties.WarehouseHost.CNameTail"
#define WAREHOUSE_HWHEALTH_ENABLE_RFC_PARAM     "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.hwHealthTest.Enable"
#define WAREHOUSE_HWHEALTH_EXECUTE_RFC_PARAM    "Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.hwHealthTest.ExecuteTest"
#define WAREHOUSE_HWHEALTH_RESULTS_RFC_PARAM    "Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.hwHealthTest.Results"

#define WAREHOUSE_METHOD_RESET_DEVICE "resetDevice"
#define WAREHOUSE_METHOD_INTERNAL_RESET "internalReset"
#define WAREHOUSE_METHOD_LIGHT_RESET "lightReset"
#define WAREHOUSE_METHOD_IS_CLEAN "isClean"
#define WAREHOUSE_METHOD_EXECUTE_HARDWARE_TEST "executeHardwareTest"
#define WAREHOUSE_METHOD_GET_HARDWARE_TEST_RESULTS "getHardwareTestResults"

#define WAREHOUSE_EVT_DEVICE_INFO_RETRIEVED "deviceInfoRetrieved"
#define WAREHOUSE_EVT_RESET_DONE "resetDone"

#define HOSTS_FILE "/etc/warehouseHosts.conf"

#define PARAM_SUCCESS "success"
#define PARAM_ERROR "error"

#define DEVICE_INFO_SCRIPT "sh /lib/rdk/getDeviceDetails.sh read"
#define VERSION_FILE_NAME "/version.txt"
#define CUSTOM_DATA_FILE "/lib/rdk/wh_api_5.conf"

#define LIGHT_RESET_SCRIPT "/opt/netflix/* SD_CARD_MOUNT_PATH/netflix/* XDG_DATA_HOME/* XDG_CACHE_HOME/* XDG_CACHE_HOME/../.sparkStorage/ /opt/QT/home/data/* /opt/hn_service_settings.conf /opt/apps/common/proxies.conf /opt/lib/bluetooth /opt/persistent/rdkservicestore"
#define INTERNAL_RESET_SCRIPT "rm -rf /opt/drm /opt/www/whitebox /opt/www/authService && /rebootNow.sh -s WarehouseService &"

#define FRONT_PANEL_NONE -1
#define FRONT_PANEL_INPROGRESS 1
#define FRONT_PANEL_FAILED 3
#define FRONT_PANEL_INTERVAL 5000

static const char WAREHOUSE_RESET_FLAG_FILE[] = "/opt/.rebootFlag";
static const int READ_BUFFER_SZ = 1024;
static const int MAX_LOG_SIZE = 128;



// TODO: remove this
#define registerMethod(...) for (uint8_t i = 1; GetHandler(i); i++) GetHandler(i)->Register<JsonObject, JsonObject>(__VA_ARGS__)

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 9

namespace Utils {
std::string formatIARMResult(IARM_Result_t result)
{
    switch (result) {
    case IARM_RESULT_SUCCESS:       return std::string("IARM_RESULT_SUCCESS [success]");
    case IARM_RESULT_INVALID_PARAM: return std::string("IARM_RESULT_INVALID_PARAM [invalid input parameter]");
    case IARM_RESULT_INVALID_STATE: return std::string("IARM_RESULT_INVALID_STATE [invalid state encountered]");
    case IARM_RESULT_IPCCORE_FAIL:  return std::string("IARM_RESULT_IPCORE_FAIL [underlying IPC failure]");
    case IARM_RESULT_OOM:           return std::string("IARM_RESULT_OOM [out of memory]");
    default:
        std::ostringstream tmp;
        tmp << result << " [unknown IARM_Result_t]";
        return tmp.str();
    }
}
bool isFileExistsAndOlderThen(const char* pFileName, long age = -1)
{
    struct stat fileStat;
    int res = stat(pFileName, &fileStat);
    if (0 != res)
        return false;

    if (-1 == age)
        return true;

    time_t currentTime = time(nullptr);

    time_t modifiedSecondsAgo = difftime(currentTime, fileStat.st_mtime);

    return modifiedSecondsAgo > age;
}
}

namespace WPEFramework
{

    namespace {

        static Plugin::Metadata<Plugin::Warehouse> metadata(
            // Version (Major, Minor, Patch)
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    namespace Plugin
    {
        SERVICE_REGISTRATION(Warehouse, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);
        Warehouse* Warehouse::_instance = nullptr;
        Warehouse::Warehouse()
        : PluginHost::JSONRPC()
#ifdef HAS_FRONT_PANEL
        , m_ledTimer(64 * 1024, "LedTimer")
        , m_ledInfo(this)
#endif
        {
            LOGWARN ("Ctor:%d", __LINE__);
            Warehouse::_instance = this;
            CreateHandler({2});

            registerMethod(WAREHOUSE_METHOD_RESET_DEVICE, &Warehouse::resetDeviceWrapper, this);
            registerMethod(WAREHOUSE_METHOD_INTERNAL_RESET, &Warehouse::internalResetWrapper, this);
            registerMethod(WAREHOUSE_METHOD_LIGHT_RESET, &Warehouse::lightResetWrapper, this);
            registerMethod(WAREHOUSE_METHOD_IS_CLEAN, &Warehouse::isCleanWrapper, this);
            registerMethod(WAREHOUSE_METHOD_EXECUTE_HARDWARE_TEST, &Warehouse::executeHardwareTestWrapper, this);
            registerMethod(WAREHOUSE_METHOD_GET_HARDWARE_TEST_RESULTS, &Warehouse::getHardwareTestResultsWrapper, this);
        }

        Warehouse::~Warehouse()
        {
            LOGWARN ("Dtor:%d", __LINE__);
        }

        const string Warehouse::Initialize(PluginHost::IShell* /* service */)
        {
            InitializeIARM();
            LOGWARN ("Warehouse::Initialize finished line:%d", __LINE__);
            return (string());
        }

        void Warehouse::Deinitialize(PluginHost::IShell* /* service */)
        {
            if (m_resetThread.get().joinable())
                m_resetThread.get().join();
            Warehouse::_instance = nullptr;
            DeinitializeIARM();
            LOGWARN ("Warehouse::Deinitialize finished line:%d", __LINE__);
        }

        void Warehouse::InitializeIARM()
        {
        if (Utils::IARM::init()) {
               IARM_Result_t res;
               IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_WAREHOUSEOPS_STATUSCHANGED, dsWareHouseOpnStatusChanged) );
            }
        }

        void Warehouse::DeinitializeIARM()
        {
            if (Utils::IARM::isConnected()) {
                IARM_Result_t res;
                IARM_CHECK( IARM_Bus_RemoveEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, dsWareHouseOpnStatusChanged) );
            }
        }

#if defined(USE_IARMBUS) || defined(USE_IARM_BUS)
        static void WareHouseResetIARM(Warehouse *wh, bool suppressReboot, const string& resetType)
        {
            IARM_Result_t err = IARM_RESULT_IPCCORE_FAIL;
            uint32_t ret = Core::ERROR_GENERAL;
            JsonObject params;
            bool isWareHouse = false;
            if (NULL == Warehouse::_instance) {
                return;
            }
            if (resetType.compare("COLD") == 0)
            {
                LOGINFO("%s reset...", resetType.c_str());
                ret = Warehouse::_instance->processColdFactoryReset();
                 if (ret == Core::ERROR_NONE) {
                    params[PARAM_SUCCESS] = true;
                    LOGINFO("%s reset... success.", resetType.c_str());
                }
                else {
                    params[PARAM_ERROR] = "Reset failed";
                }
            }
            else if (resetType.compare("FACTORY") == 0)
            {
                LOGINFO("%s reset...", resetType.c_str());
                ret = Warehouse::_instance->processFactoryReset();
                 if (ret == Core::ERROR_NONE) {
                    params[PARAM_SUCCESS] = true;
                    LOGINFO("%s reset... success.", resetType.c_str());
                }
                else {
                    params[PARAM_ERROR] = "Reset failed";
                }
            }
            else if (resetType.compare("USERFACTORY") == 0)
            {
                LOGINFO("%s reset...", resetType.c_str());
                ret = Warehouse::_instance->processUserFactoryReset();
                 if (ret == Core::ERROR_NONE) {
                    params[PARAM_SUCCESS] = true;
                    LOGINFO("%s reset... success.", resetType.c_str());
                }
                else {
                    params[PARAM_ERROR] = "Reset failed";
                }
            }
            else if (resetType.compare("WAREHOUSE_CLEAR") == 0)
            {
                LOGINFO("%s reset...", resetType.c_str());
                ret = suppressReboot ?  Warehouse::_instance->processWHClearNoReboot(): Warehouse::_instance->processWHClear();
                if (ret == Core::ERROR_NONE) {
                    isWareHouse = true;
                    LOGINFO("%s reset... success.", resetType.c_str());
                }
                else {
                    params[PARAM_ERROR] = "Reset failed";
                }
            }
            else // WAREHOUSE
            {
                LOGINFO("WAREHOUSE reset...");
                ret = suppressReboot ?  Warehouse::_instance->processWHResetNoReboot(): Warehouse::_instance->processWHReset();
                if (ret == Core::ERROR_NONE) {
                    isWareHouse = true;
                }
                else {
                    params[PARAM_ERROR] = "Reset failed";
                }
            }

            bool ok = true;

            // If SuppressReboot is false ie device is rebooted, IARMBus daemon will also be stopped.
            // So cannot rely on return value from IARM_Bus_Call above.
            // Hence checking for status only when SuppressReboot is true.
            if(suppressReboot)
            {
                ok = ((err == IARM_RESULT_SUCCESS)|| (ret == Core::ERROR_NONE));
            }
  
            if (!( true == isWareHouse && true == suppressReboot)) {
                JsonObject params;
                params[PARAM_SUCCESS] = ok;

                if (!ok)
                {
                    LOGWARN("%s", C_STR(Utils::formatIARMResult(err)));
                    params[PARAM_ERROR] = "Reset failed";
                }
                string json;
                params.ToString(json);
                LOGINFO("Notify %s %s\n", WAREHOUSE_EVT_RESET_DONE, json.c_str());
                wh->Notify(WAREHOUSE_EVT_RESET_DONE, params);
            }
        }
#endif

        void Warehouse::resetDevice(bool suppressReboot, const string& resetType)
        {
            JsonObject params;
#if defined(USE_IARMBUS) || defined(USE_IARM_BUS)

            LOGWARN("Received request to reset device");

            try
            {
                if (m_resetThread.get().joinable())
                    m_resetThread.get().join();

                m_resetThread = Utils::ThreadRAII(std::thread(WareHouseResetIARM, this, suppressReboot, resetType));
            }
            catch(const std::system_error& e)
            {
                LOGERR("system_error exception in thread join %s", e.what());
                params[PARAM_SUCCESS] = false;
                params[PARAM_ERROR] = "exception in submitting request";
                sendNotify(WAREHOUSE_EVT_RESET_DONE, params);
            }
            catch(const std::exception& e)
            {
                LOGERR("exception in thread join %s", e.what());
                params[PARAM_SUCCESS] = false;
                params[PARAM_ERROR] = "exception in submitting request";
                sendNotify(WAREHOUSE_EVT_RESET_DONE, params);
            }
#else
            bool ok = false;
            params[PARAM_SUCCESS] = ok;

            if (!ok)
                params[PARAM_ERROR] = "No IARMBUS";

            sendNotify(WAREHOUSE_EVT_RESET_DONE, params);
#endif
        }

#ifdef HAS_FRONT_PANEL
        static bool SetFrontPanelLights(int state, int iteration)
        {
            bool ledData = false;
            bool ledRecord = false;
            int ledBrightness = 100;
            if (state == FRONT_PANEL_INPROGRESS)
            {
                // i    0   1   2   3   4   5   6   7   8
                // DA   1   1   1   1   0   0   0   0   1
                // RE   0   0   0   0   1   1   1   1   0
                // BR   100 75  50  25  0   25  50  75  100
                ledData = (iteration / 4) % 2 == 0;
                ledRecord = !ledData;
                ledBrightness = abs(100 - 25 * (iteration % 8));
            }
            else if (state == FRONT_PANEL_FAILED)
            {
                // i    0   1   2
                // DA   1   0   1
                // RE   0   1   0
                // BR   100 100 100
                ledData = iteration % 2 == 0;
                ledRecord = !ledData;
                ledBrightness = 100;
            }
            LOGINFO("SetFrontPanelLights set Brightness=%d (LEDs: Data=%d Record=%d)",
                       ledBrightness, ledData?1:0, ledRecord?1:0);
            CFrontPanel* helper = CFrontPanel::instance();
            bool didSet = false;
            if (ledData)
                didSet |= helper->powerOnLed(FRONT_PANEL_INDICATOR_MESSAGE);
            else
                didSet |= helper->powerOffLed(FRONT_PANEL_INDICATOR_MESSAGE);
            if (ledRecord)
                didSet |= helper->powerOnLed(FRONT_PANEL_INDICATOR_RECORD);
            else
                didSet |= helper->powerOffLed(FRONT_PANEL_INDICATOR_RECORD);
            didSet |= helper->setBrightness(ledBrightness);
            return didSet;
        }
#endif

        
        void Warehouse::internalReset(JsonObject& response)
        {
            bool isProd = false;

            if (0 == access(VERSION_FILE_NAME, R_OK))
            {
                std::ifstream file(VERSION_FILE_NAME);

                LOGWARN("%s: attempt to open file '%s' for read mode with result %s", __FUNCTION__
                        , VERSION_FILE_NAME, file.good() ? "sucess" : "failure");

                std::string line;
                while(std::getline(file, line))
                {
                    if (line.find("imagename:") != std::string::npos)
                    {
                        isProd = line.find("PROD") != std::string::npos;
                        break;
                    }
                }
            }

            if (isProd)
            {
                response[PARAM_SUCCESS] = false;
                response[PARAM_ERROR] = "version is PROD";
            }
            else
            {
#if defined(USE_IARMBUS) || defined(USE_IARM_BUS)
                std::string error;
                int return_value = v_secure_system("rm -rf /opt/drm /opt/www/whitebox /opt/www/authService && /rebootNow.sh -s WarehouseService &");
                bool ok = return_value == 0;
                response[PARAM_SUCCESS] = ok;
                if (!ok)
                    response[PARAM_ERROR] = error;
#else
                response[PARAM_SUCCESS] = false;
                response[PARAM_ERROR] = "No IARMBUS";
#endif
            }
        }

        void Warehouse::lightReset(JsonObject& response)
        {
#if defined(USE_IARMBUS) || defined(USE_IARM_BUS)

            std::string script(LIGHT_RESET_SCRIPT);
            regex_t rx;
            regcomp(&rx, "(\\s+)([A-Z_][0-9A-Z_]*)(\\S*)", REG_EXTENDED);
            regmatch_t rm[4];

            int pos = 0;
            while (regexec(&rx, script.c_str() + pos, 4, rm, 0) == REG_NOERROR)
            {
                std::string var = script.substr(pos + rm[2].rm_so, rm[2].rm_eo - rm[2].rm_so);
                std::string replace;
                const char *envVar = getenv(var.c_str());
                string scmp;

                if ("SD_CARD_MOUNT_PATH" == var && (!envVar || 0 == *envVar))
                {
                    /*getting the mount path from function if unable to get from env variables*/
                    if (getSDCardMountPath(scmp) == true)
                    {
                        scmp.erase(0, scmp.find_first_not_of(" \n\r\t"));
                        scmp.erase(scmp.find_last_not_of(" \n\r\t") + 1);
                        envVar = scmp.c_str();
                    }
                    else
                    {
                         LOGWARN("failed to get SD_CARD_MOUNT_PATH");
                    }
                }

                if (envVar && *envVar)
                {
                    replace += envVar;
                    replace += script.substr(pos + rm[3].rm_so, rm[3].rm_eo - rm[3].rm_so);
                    if (replace.size() > 0)
                    {
                        if ('/' == replace[0])
                        {
                            size_t nr = replace.find_first_not_of('/');
                            if (string::npos != nr && '*' != replace[nr]) // Check if that is not root dir
                                replace = script.substr(pos + rm[1].rm_so, rm[1].rm_eo - rm[1].rm_so) + replace;
                            else 
                                replace = "";
                        }
                        else
                            replace = script.substr(pos + rm[1].rm_so, rm[1].rm_eo - rm[1].rm_so) + replace;
                    }
                }
                script.replace(pos + rm[0].rm_so, rm[0].rm_eo - rm[0].rm_so, replace);
                pos += rm[0].rm_so + replace.size();
            }

            regfree(&rx);
            LOGWARN("lightReset: %s", script.c_str());

            std::string error;
            int return_value = v_secure_system("sh -c 'rm -rf " LIGHT_RESET_SCRIPT "'");
            bool ok = return_value == 0;

            (void)remove("/opt/secure/persistent/rdkservicestore");

            response[PARAM_SUCCESS] = ok;
            if (ok)
            {
                LOGWARN("lightReset succeeded");
            }
            else
            {
                LOGERR("lightReset failed. %s", error.c_str());
                response[PARAM_ERROR] = error;
            }
#else
            LOGERR("lightReset failed: No IARMBUS");
            response[PARAM_SUCCESS] = false;
            response[PARAM_ERROR] = "No IARMBUS";
#endif
        }

        void Warehouse::isClean(int age, JsonObject& response)
        {
            std::ifstream customDataFile(CUSTOM_DATA_FILE);
            JsonArray existedObjects;

            if(!customDataFile)
            {
                LOGERR("Can't open file %s", CUSTOM_DATA_FILE);
                response[PARAM_SUCCESS] = false;
                response[PARAM_ERROR] = "Can't open file " CUSTOM_DATA_FILE;
                response["clean"] = false;
                response["files"] = existedObjects;
                return;
            }

            std::vector<std::string> listPathsToRemove;
            for( std::string line; getline( customDataFile, line ); )
            {
                Utils::String::trim(line);
                if (line.length() > 0)
                {
                    char firstChar = line[0];
                    if (firstChar != '#' && firstChar != '[' && line.substr(0, 2) != ". ")
                        listPathsToRemove.emplace_back(line);
                }
            }
            customDataFile.close();

            if (listPathsToRemove.size() == 0)
            {
                std::string msg = "file " CUSTOM_DATA_FILE " doesn't have any lines with paths";
                LOGERR("%s", msg.c_str());
                response[PARAM_ERROR] = msg;
                response[PARAM_SUCCESS] = false;
                response["files"] = existedObjects;
                return;
            }

            int totalPathsCounter = 0;
            for(auto &path : listPathsToRemove)
            {
                // if script's variable in path is empty, then skip it
                if (path.find('$') != std::string::npos)
                {
                    std::string variable;
                    std::regex pattern("\\$([^$\\{\\}\\/]*)");
                    std::smatch matches;

                    if (std::regex_search(path, matches, pattern))
                    {
                        variable = matches[1].str();
                        LOGINFO("Extracted variable '%s' ", variable.c_str());
                        Utils::String::trim(variable);
                    }
                    else
                    {
                         LOGERR("Variable extraction failed");
                    }

                    std::string value;
                    if (variable.length() > 0)
                    {
                        const char* filename = "/etc/device.properties";
                        std::string propertyName = variable;

                        if (Utils::readPropertyFromFile(filename, propertyName, value))
                        {
                            Utils::String::trim(value);
                        }
                        else
                        {
                            LOGERR("Property value is empty");
                        }
                    }

                    if (value.length() == 0)
                    {
                        LOGWARN("path %d '%s' hasn't been tested, due to the empty value of '%s'", ++totalPathsCounter, path.c_str(), variable.c_str());
                        continue;
                    }

                    LOGINFO("variable '%s' has value '%s'", variable.c_str(), value.c_str());
                }

                if (std::find_if(path.begin(), path.end(), [](char c) { return c == '$' || c == '*' || c == '?' || c == '+'; } ) != path.end())
                {
                    std::list<std::string> exclusions;

                    if (path.find('|') != string::npos)
                    {
                        size_t last = 0, next = 0;
                        while ((next = path.find('|', last)) != string::npos)
                        {
                            std::string s = path.substr(last, next - last);
                            Utils::String::trim(s);
                            if (s.length() > 0)
                                exclusions.push_back(s);
                            last = next + 1;
                        }
                        std::string s = path.substr(last, next - last);
                        Utils::String::trim(s);
                        if (s.length() > 0)
                            exclusions.push_back(s);
                        path = exclusions.front();
                    }

                    // allow search recursively if path ends by '/*[|...]', otherwise, for cases like /*.ini, searching process will be done only by given path
                    std::string result;
                    const char* input =  path.c_str();
                    const char* filePath = "/etc/device.properties";
                    std::string inputPath = path;

                    bool success = Utils::ExpandPropertiesInString(input, filePath, inputPath);
                    if(!success)
                    {
                        LOGERR("Path String Expansion failed.\n");
                    }
                    else
                    {
                        LOGINFO("Expanded String:  %s\n ", inputPath.c_str());
                    }

                    int maxDepth = 1;
                    int mindepth = 1;
                    if (inputPath.find("/*", path.length() - 2) != std::string::npos)
                        maxDepth = 0;
                    if (Utils::searchFiles(inputPath, maxDepth, mindepth, exclusions,result))
                    {
                        LOGINFO("searchResult is %s ", result.c_str());
                        Utils::String::trim(result);
                    }
                    else
                    {
                        LOGERR("Error: %s", result.c_str());
                    }
                    
                    totalPathsCounter++;
                    if (result.length() > 1)
                    {
                        std::stringstream ss(result);
                        std::string line;
                        while(getline( ss, line, '\n' ))
                        {
                            if (age > -1)
                            {
                                bool objectExists = Utils::isFileExistsAndOlderThen(line.c_str(), (long)age);
                                if (objectExists)
                                    existedObjects.Add(line);

                                std::string strAge = std::to_string(age) + " seconds";
                                LOGINFO("object by path %d: '%s' : '%s' %s", totalPathsCounter, path.c_str(), line.c_str()
                                        , objectExists
                                        ? (std::string("exists and was modified more than ") + strAge + " ago").c_str()
                                        : (std::string("doesn't exist or was modified in ") + strAge).c_str());
                            }
                            else
                            {
                                existedObjects.Add(line);
                                LOGINFO("object by path %d: '%s' : '%s' exists", totalPathsCounter, path.c_str(), line.c_str());
                            }
                        }
                    }
                    else
                        LOGINFO("objects by path %d: '%s' don't exist", totalPathsCounter, path.c_str());
                }
                else
                {
                    bool objectExists = Utils::isFileExistsAndOlderThen(path.c_str(), (long)age);
                    if (objectExists)
                        existedObjects.Add(path);

                    totalPathsCounter++;
                    if (age > -1)
                    {
                        std::string strAge = std::to_string(age) + " seconds";
                        LOGINFO("object by path %d: '%s' %s", totalPathsCounter, path.c_str()
                                , objectExists
                                ? (std::string("exists and was modified more than ") + strAge + " ago").c_str()
                                : (std::string("doesn't exist or was modified in ") + strAge).c_str());
                    }
                    else
                        LOGINFO("object by path %d: '%s' %s", totalPathsCounter, path.c_str(), objectExists ? "exists" : "doesn't exist");
                }
            }

            LOGINFO("checked %d paths, found %d objects", totalPathsCounter, (int)existedObjects.Length());
            response[PARAM_SUCCESS] = true;
            response["files"] = existedObjects;
            response["clean"] = existedObjects.Length() == 0;
        }

        bool Warehouse::executeHardwareTest() const
        {
            bool result = false;

            WDMP_STATUS wdmpStatus;

            wdmpStatus = setRFCParameter((char *)WAREHOUSE_RFC_CALLERID, WAREHOUSE_HWHEALTH_ENABLE_RFC_PARAM, "true", WDMP_BOOLEAN);
            result = (wdmpStatus == WDMP_SUCCESS);
            if (result)
            {
                wdmpStatus = setRFCParameter((char *)WAREHOUSE_RFC_CALLERID, WAREHOUSE_HWHEALTH_EXECUTE_RFC_PARAM, "1", WDMP_INT);
                result = (wdmpStatus == WDMP_SUCCESS);
            }
            if (!result)
                LOGERR("%s", getRFCErrorString(wdmpStatus));

            return result;
        }

        bool Warehouse::getHardwareTestResults(string& testResults) const
        {
            bool result = false;

            WDMP_STATUS wdmpStatus;
            RFC_ParamData_t param = {0};

            wdmpStatus = getRFCParameter((char *)WAREHOUSE_RFC_CALLERID, WAREHOUSE_HWHEALTH_RESULTS_RFC_PARAM, &param);
            result = (wdmpStatus == WDMP_SUCCESS);
            if (result)
            {
                testResults = param.value;
                wdmpStatus = setRFCParameter((char *)WAREHOUSE_RFC_CALLERID, WAREHOUSE_HWHEALTH_ENABLE_RFC_PARAM, "false", WDMP_BOOLEAN);
                result = (wdmpStatus == WDMP_SUCCESS);
            }
            if (!result)
                LOGERR("%s", getRFCErrorString(wdmpStatus));

            return result;
        }

        // JsonRpc
        uint32_t Warehouse::resetDeviceWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();

            bool suppressReboot = false;
            if (parameters.HasLabel("suppressReboot"))
            {
                getBoolParameter("suppressReboot", suppressReboot);
            }

            string resetType;
            if (parameters.HasLabel("resetType"))
            {
                getStringParameter("resetType", resetType);
            }

            resetDevice(suppressReboot, resetType);

            response[PARAM_SUCCESS] = true;
            returnResponse(true);
        }

        uint32_t Warehouse::internalResetWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();

            if (parameters.HasLabel("passPhrase") && parameters["passPhrase"].String() == "FOR TEST PURPOSES ONLY")
            {
                internalReset(response);
                return (Core::ERROR_NONE); 
            }
            else
            {
                response[PARAM_SUCCESS] = false;
                response[PARAM_ERROR] = "incorrect pass phrase";
                returnResponse(false);
            }
        }

        uint32_t Warehouse::lightResetWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();

            lightReset(response);
            return Core::ERROR_NONE;
        }

        uint32_t Warehouse::isCleanWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();

            int age;
            getDefaultNumberParameter("age", age, -1);

            isClean(age, response);
            return Core::ERROR_NONE;
        }

        uint32_t Warehouse::executeHardwareTestWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();

            bool success = false;

            success = executeHardwareTest();

            returnResponse(success);
        }

        uint32_t Warehouse::getHardwareTestResultsWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();

            bool success = false;

            string testResults;
            success = getHardwareTestResults(testResults);
            if (success)
                response["testResults"] = testResults;

            returnResponse(success);
        }

#ifdef HAS_FRONT_PANEL
        void Warehouse::onSetFrontPanelStateTimer()
        {
            SetFrontPanelLights(m_ledState, m_ledTimerIteration);
            ++m_ledTimerIteration;
            m_ledTimer.Schedule(Core::Time::Now().Add(FRONT_PANEL_INTERVAL), m_ledInfo);
        }

        uint64_t LedInfo::Timed(const uint64_t scheduledTime)
        {
            uint64_t result = 0;
            m_warehouse->onSetFrontPanelStateTimer();
            return(result);
        }
#endif
        void Warehouse::dsWareHouseOpnStatusChanged(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if (NULL == Warehouse::_instance) {
                return;
            }
            if (IARM_BUS_PWRMGR_EVENT_WAREHOUSEOPS_STATUSCHANGED == eventId)
            {
		IARM_BUS_PWRMgr_WareHouseOpn_EventData_t *eventData = (IARM_BUS_PWRMgr_WareHouseOpn_EventData_t *) data;
		if (NULL == eventData) {
                    LOGWARN("dsWareHouseOpnStatusChanged eventData is NULL. exiting");
		    //Unexpected case
		    return;
		}
		JsonObject params;
		if (IARM_BUS_PWRMGR_WAREHOUSE_COMPLETED == eventData->status) {
                    params[PARAM_SUCCESS] = true;
		}
		else {
                    params[PARAM_ERROR] = "Reset failed";
		}
                string json;
		params.ToString(json);
		LOGINFO("Notify %s %s\n", WAREHOUSE_EVT_RESET_DONE, json.c_str());
		Warehouse::_instance->Notify(WAREHOUSE_EVT_RESET_DONE, params);
            }
        }

        uint32_t Warehouse::processColdFactoryReset()
        {
            /*Code copied from X1.. Needs modification*/
            LOGINFO("\n Reset: Processing Cold Factory Reset\n");
            fflush(stdout);
            LOGINFO(" Reset: ...Clearing data from your box before reseting \n");
            fflush(stdout);
            /*Execute the script for Cold Factory Reset*/
            v_secure_system("sh /lib/rdk/deviceReset.sh coldfactory");
            resetWarehouseRebootFlag();
            sleep(5);
            v_secure_system(" /rebootNow.sh -s PowerMgr_coldFactoryReset -o 'Rebooting the box due to Cold Factory Reset process ...'");
            return Core::ERROR_NONE;
        }

        uint32_t Warehouse::processFactoryReset()
        {
            /*Code copied from X1.. Needs modification*/
            LOGINFO("\n Reset: Processing Factory Reset\n");
            fflush(stdout);
            LOGINFO("Reset: ...Clearing data from your box before reseting \n");
            fflush(stdout);
            /*Execute the script for Factory Reset*/
            v_secure_system("sh /lib/rdk/deviceReset.sh factory");
            resetWarehouseRebootFlag();
            return Core::ERROR_NONE;
        }

        uint32_t Warehouse::processWHReset()
        {
            LOGINFO("\n Reset: Processing Ware House Reset\n");
            fflush(stdout);
            /*Execute the script for Ware House Reset*/
            resetWarehouseRebootFlag();
            std::ofstream { "/tmp/.warehouse-reset" };
            return v_secure_system("sh /lib/rdk/deviceReset.sh warehouse");
        }

        uint32_t Warehouse::processWHClear()
        {
            /*Code copied from X1.. Needs modification*/
            LOGINFO("\n Clear: Processing Ware House Clear\n");
            fflush(stdout);
            resetWarehouseRebootFlag();
            std::ofstream { "/tmp/.warehouse-clear" };
            v_secure_system("sh /lib/rdk/deviceReset.sh WAREHOUSE_CLEAR");
            return Core::ERROR_NONE;
        }

        uint32_t Warehouse::processWHClearNoReboot()
        {
            LOGINFO("\n Clear: Invoking Ware House Clear Request from APP\n");
            fflush(stdout);
            std::ofstream { "/tmp/.warehouse-clear" };
            return v_secure_system("sh /lib/rdk/deviceReset.sh WAREHOUSE_CLEAR --suppressReboot");
        }

        uint32_t Warehouse::processWHResetNoReboot()
        {
            LOGINFO("\n Reset: Invoking Ware House Reset Request from APP\n");
            fflush(stdout);
            /*Execute the script for Ware House Reset*/
            std::ofstream { "/tmp/.warehouse-reset" };
            return v_secure_system("sh /lib/rdk/deviceReset.sh warehouse --suppressReboot &");
        }

        uint32_t Warehouse::processUserFactoryReset()
        {
            LOGINFO("\n Reset: Processing User Factory Reset\n");
            fflush(stdout);
            /*Execute the script for User Factory Reset*/
            resetWarehouseRebootFlag();
            v_secure_system("sh /lib/rdk/deviceReset.sh userfactory");
            return Core::ERROR_NONE;
        }

        void Warehouse::resetWarehouseRebootFlag()
        {
            FILE *fp = fopen(WAREHOUSE_RESET_FLAG_FILE, "w");
            if (fp != NULL)
            {
                fputs("0\n", fp);
                fclose(fp);
            }
            else
            {
                LOGWARN ("Warehouse::ResetWarehouseRebootFlag failed\n");
            }
        }

        void Warehouse::getDateAndTime(string& utcDateTime)
        {
            char timeStringBuffer[128] = {'\0'};
            time_t rawTime = time(0);
            struct tm *gmt = gmtime(&rawTime);
            strftime(timeStringBuffer, sizeof(timeStringBuffer), "%d.%m.%Y_%H.%M.%S", gmt);
            utcDateTime = timeStringBuffer;
            return;
        }

        bool Warehouse::getSDCardMountPath(string& mount_path)
        {
            char buffer[READ_BUFFER_SZ];
            FILE* file = fopen("/proc/mounts", "r");
            bool found_keyword =false;

            if (file)
            {
                while (fgets(buffer, sizeof(buffer), file) != NULL)
                {
                    char *line = strstr(buffer, "mmcblk0p1");
                    char *line_end;
                    if (line != nullptr)
                    {
                        // Skip the matching string
                        while (*line && *line != ' ')
                           line++;
                        // Skip any spaces
                        while (*line == ' ')
                           line++;
                        // Terminate the string at the first space after the path
                        for (line_end = line; *line_end && *line_end != ' '; line_end++);
                        *line_end = 0;
                        mount_path = line;
                        found_keyword = true;
                        break;
                    }
                }
                fclose(file);
            }
            else
            {
                LOGERR("Failed to open /proc/mounts");
            }
            return found_keyword;
        }
    } // namespace Plugin
} // namespace WPEFramework
