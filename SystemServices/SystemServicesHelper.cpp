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

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sys/stat.h>
#include <algorithm>
#include <curl/curl.h>

#include "SystemServicesHelper.h"

#include "UtilsLogging.h"
#include "UtilsfileExists.h"
#include "UtilsFile.h"
#include "UtilsString.h"
#include "UtilsgetFileContent.h"
#include <secure_wrapper.h>

#define GET_STB_DETAILS_SCRIPT_READ_COMMAND "read"

/* Helper Functions */
using namespace std;

static const std::map<int, std::string> ErrCodeMap = {
    {SysSrv_OK, "Processed Successfully"},
    {SysSrv_MethodNotFound, "Method not found"},
    {SysSrv_MissingKeyValues, "Missing required key/value(s)"},
    {SysSrv_UnSupportedFormat, "Unsupported or malformed format"},
    {SysSrv_FileNotPresent, "Expected file not found"},
    {SysSrv_FileAccessFailed, "File access failed"},
    {SysSrv_FileContentUnsupported, "Unsupported file content"},
    {SysSrv_Unexpected, "Unexpected error"},
    {SysSrv_SupportNotAvailable, "Support not available/enabled"},
    {SysSrv_LibcurlError, "LIbCurl service error"},
    {SysSrv_DynamicMemoryAllocationFailed, "Dynamic Memory Allocation Failed"},
    {SysSrv_ManufacturerDataReadFailed, "Manufacturer Data Read Failed"},
    {SysSrv_KeyNotFound, "Key not found"}
};

std::string getErrorDescription(int errCode)
{
    std::string errMsg = "Unexpected Error";

    auto it = ErrCodeMap.find(errCode);
    if (ErrCodeMap.end() != it) {
        errMsg = it->second;
    }
    return errMsg;
}

std::string dirnameOf(const std::string& fname)
{
    size_t pos = fname.find_last_of("/");
    return (std::string::npos == pos) ? "" : fname.substr(0, pos+1);
}

/***
 * @brief	: Used to check if directory exists
 * @param1[in]	: Complete file name with path
 * @param2[in]	: Destination string to be filled with file contents
 * @return	: <bool>; TRUE if operation success; else FALSE.
 */
bool dirExists(std::string fname)
{
    bool status = false;
    struct stat sb;
    if (stat((dirnameOf(fname)).c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        status = true;
    } else {
        // Do nothing.
    }
    return status;
}

/***
 * @brief	: Used to read file contents into a string
 * @param1[in]	: Complete file name with path
 * @param2[in]	: Destination string to be filled with file contents
 * @return	: <bool>; TRUE if operation success; else FALSE.
 */
bool readFromFile(const char* filename, string &content)
{
    bool retStatus = false;

    if (!Utils::fileExists(filename)) {
        return retStatus;
    }

    ifstream ifile(filename, ios::in);
    if (ifile.is_open()) {
        std::getline(ifile, content);
        ifile.close();
        retStatus = true;
    } else {
        retStatus = false;
    }

    return retStatus;
}

namespace WPEFramework {
    namespace Plugin {
        /***
         * @brief	: Used to construct response with module error status.
         * @param1[in]	: Error Code
         * @param2[out]: "response" JSON Object which is returned by the API
         with updated module error status.
         */
        void populateResponseWithError(int errorCode, JsonObject& response)
        {
            if (errorCode) {
                LOGWARN("Method %s failed; reason : %s\n", __FUNCTION__,
                        getErrorDescription(errorCode).c_str());
                response["SysSrv_Status"] = static_cast<uint32_t>(errorCode);
                response["errorMessage"] = getErrorDescription(errorCode);
            }
        }

        string caseInsensitive(string str7)
        {
            string status = "ERROR";
            regex r("model=([^\r\n]*)\n?",
                    std::regex_constants::ECMAScript | std::regex_constants::icase);
            regex r1("model_number=([^\r\n]*)\n?",
                    std::regex_constants::ECMAScript | std::regex_constants::icase);
            smatch match;

            if (regex_search(str7, match, r) == true) {
                status = match.str(1);
            } else if (regex_search(str7, match, r1) == true) {
                status = match.str(1);
            } else {
                // Do nothing
                ;
            }
            return status;
        }

        string ltrim(const string& s)
        {
            const string WHITESPACE = " \n\r\t\f\v";
            size_t start = s.find_first_not_of(WHITESPACE);
            return (start == string::npos) ? "" : s.substr(start);
        }

        string rtrim(const string& s)
        {
            const string WHITESPACE = " \n\r\t\f\v";
            size_t end = s.find_last_not_of(WHITESPACE);
            return (end == string::npos) ? "" : s.substr(0, end + 1);
        }

        string trim(const string& s)
        {
            return rtrim(ltrim(s));
        }

        string getModel()
        {
            FILE* pipe = v_secure_popen("r", "/lib/rdk/getDeviceDetails.sh %s", GET_STB_DETAILS_SCRIPT_READ_COMMAND);
            LOGWARN("%s: opened pipe for command /lib/rdk/getDeviceDetails.sh, with result %s : %s\n",
                    __FUNCTION__ , pipeName, pipe ? "success" : "failure");
            if (!pipe) {
                LOGERR("%s: SERVICEMANAGER_FILE_ERROR: Can't open pipe for command /lib/rdk/getDeviceDetails.sh for read mode: %s\n"
                        , __FUNCTION__, strerror(errno));
                return "ERROR";
            }

            char buffer[128] = {'\0'};
            string result;

            while (!feof(pipe)) {
                if (fgets(buffer, 128, pipe) != NULL) {
                    result += buffer;
                }
            }
            v_secure_pclose(pipe);

            string tri = caseInsensitive(result);
            string ret = tri.c_str();
            ret = trim(ret);
            LOGWARN("%s: ret=%s\n", __FUNCTION__, ret.c_str());
            return ret;
        }

        string convertCase(string str)
        {
            std::string bufferString = str;
            transform(bufferString.begin(), bufferString.end(),
                    bufferString.begin(), ::toupper);
            LOGWARN("%s: after transform to upper :%s\n", __FUNCTION__,
                    bufferString.c_str());
            return bufferString;
        }

        bool convert(string str3, string firm)
        {
            LOGWARN("INSIDE CONVERT\n");
            bool status = false;
            string firmware = convertCase(firm);
            string str = firmware.c_str();
            size_t found = str.find(str3);
            if (found != string::npos) {
                status = true;
            } else {
                status = false;
            }
            return status;
        }
    } //namespace Plugin
} //namespace WPEFramework

/***
 * @brief	: Used to construct JSON response from Vector.
 * @param1[in]	: Destination JSON response buffer
 * @param2[in]	: JSON "Key"
 * @param3[in]	: Source Vector.
 * @return	: <bool>; TRUE if operation success; else FALSE.
 */
void setJSONResponseArray(JsonObject& response, const char* key,
        const vector<string>& items)
{
    JsonArray arr;

    for (auto& i : items) {
        arr.Add(JsonValue(i));
    }
    response[key] = arr;
}

/***
 * @brief	: Used to read file contents into a string
 * @param1[in]	: Complete file name with path
 * @param2[out]	: Destination string object filled with file contents
 * @return	: <bool>; TRUE if operation success; else FALSE.
 */
bool getFileContent(std::string fileName, std::string& fileContent)
{
    std::ifstream inFile(fileName.c_str(), ios::in);
    if (!inFile.is_open()) return false;

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    fileContent = buffer.str();
    inFile.close();

    return true;
}

/***
 * @brief	: Used to read file contents into a vector
 * @param1[in]	: Complete file name with path
 * @param2[in]	: Destination vector buffer to be filled with file contents
 * @return	: <bool>; TRUE if operation success; else FALSE.
 */
bool getFileContent(std::string fileName, std::vector<std::string> & vecOfStrs)
{
    bool retStatus = false;
    std::ifstream inFile(fileName.c_str(), ios::in);

    if (!inFile.is_open())
        return retStatus;

    std::string line;
    retStatus = true;
    while (std::getline(inFile, line)) {
        if (line.size() > 0) {
            vecOfStrs.push_back(line);
        }
    }
    inFile.close();
    return retStatus;
}

bool getDownloadProgress(int& downloadPercent)
{
    bool retStatus = false;
    string file_content = "";
    string last_line = "";
    string downloadprogress = "";
    vector<string> stringList;
    string str = "";

    /* read the file contents, which is equivalent to "cat /opt/curl_progress" */
    retStatus = Utils::readFileContent(DOWNLOAD_PROGRESS_FILE, file_content);
    LOGERR("file_content [%s].", file_content.c_str());
    if (retStatus == true && (!file_content.empty()))
    {
        /* get the last non empty line, which is equivalent to "tr -s '\r' '\n' | tail -n 1" */
        retStatus = Utils::getLastLine(file_content, last_line);
        if (retStatus == true && (!last_line.empty()))
        {
              retStatus = false;
              // trim the leading spaces, which is equivalent to "sed 's/^ *//g'"
              Utils::String::ltrim(last_line);
              // filter lines which has 'M' or 'G'  sed '/^[^M/G]*$/d', which is equivalent to "sed '/^[^M/G]*$/d'"
              std::size_t found_M = last_line.find_first_of("M");
              std::size_t found_G = last_line.find_first_of("G");
              if ((found_M != std::string::npos) | (found_G != std::string::npos))
              {
                  /* Remove extra whitespaces from given input string, which is equivalent to "tr -s ' '" */
                  Utils::String::removeExtraWhitespaces(last_line, str);

                  /* Divide the input string into words with delimiter(space) and get the third word,
                     which is equivalent to "cut -d ' ' -f3" */
                  Utils::String::split(stringList, str, " ");
                  if ((!stringList[2].empty()))
                  {
                      downloadprogress = stringList[2];
                      retStatus = true;
                  }
              }
        }
    }

    LOGINFO("downloadprogress [%s]", downloadprogress.c_str());
    if (retStatus == true && !downloadprogress.empty())
    {
        downloadPercent = strtol(downloadprogress.c_str(), NULL, 10);
        LOGINFO("FirmwareDownloadPercent = [%d]\n", downloadPercent);
    }
    else
    {
        LOGERR("Cannot read FirmwareDownloadPercent\n");
    }
    return retStatus;
}

/***
 * @brief  : compare two C string case insensitively
 * @param1[in] : c string 1
 * @param2[in] : c string 2
 * @return : <bool>; 0 is strings are same, some number if strings are different.
 */
int strcicmp(char const *a, char const *b)
{
    int d = -1;
    for (;; a++, b++) {
        d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a) {
            return d;
        }
    }
    return -1;
}

/**
 * @brief	: To find case insensitive substring in a given string.
 * @param1[in]	: Haystack buffer
 * @param2[in]	: Needle to be searched in Haystack
 * @return	: <bool> ; TRUE if match found; else FALSE.
 */
bool findCaseInsensitive(std::string data, std::string toSearch, size_t pos)
{
    /* Convert case to same type (lowercase). */
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    std::transform(toSearch.begin(), toSearch.end(), toSearch.begin(), ::tolower);
    return ((data.find(toSearch, pos) != std::string::npos)? true : false);
}

/***
 * @brief	: To retrieve Xconf version of URL to override
 * @param1[out]	: bFileExists - Returns true if /opt/swupdate.conf is present
 * @return	: string
 */
string getXconfOverrideUrl(bool& bFileExists)
{
    string xconfUrl = "";
    vector<string> lines;
    bFileExists = false;

    if (!Utils::fileExists(XCONF_OVERRIDE_FILE)) {
        return xconfUrl;
    }

    bFileExists = true;

    if (getFileContent(XCONF_OVERRIDE_FILE, lines)) {
        if (lines.size()) {
            for (int i = 0; i < (int)lines.size(); ++i) {
                string line = lines.at(i);
                if (!line.empty() && (line[0] != '#')) {
                    xconfUrl = line;
                }
            }
        }
    }
    return xconfUrl;
}

/***
 * @brief	: To retrieve TimeZone
 * @return	: string; TimeZone
 */
string getTimeZoneDSTHelper(void)
{
    string timeZone = "";
    vector<string> lines;

    if (!Utils::fileExists(TZ_FILE)) {
        return timeZone;
    }

    if (getFileContent(TZ_FILE, lines)) {
        if (lines.size() > 0) {
            timeZone = lines.front();
        }
    }

    return timeZone;
}

/***
 * @brief	: To retrieve TimeZone Accuracy
 * @return	: string; TimeZone Accuracy
 */
string getTimeZoneAccuracyDSTHelper(void)
{
    string timeZoneAccuracy = "";
    vector<string> lines;

    if (!Utils::fileExists(TZ_ACCURACY_FILE)) {
        return TZ_ACCURACY_INITIAL;
    }

    if (getFileContent(TZ_ACCURACY_FILE, lines)) {
        if (lines.size() > 0) {
            timeZoneAccuracy = lines.front();
        }
    }

    if (timeZoneAccuracy != TZ_ACCURACY_INITIAL && timeZoneAccuracy != TZ_ACCURACY_INTERIM && timeZoneAccuracy != TZ_ACCURACY_FINAL) {
        LOGWARN("Got empty or invalid TimeZone Accuracy from the file, using default 'INITIAL'");
        timeZoneAccuracy = TZ_ACCURACY_INITIAL;
    }

    return timeZoneAccuracy;
}

/***
 * @brief		: To retrieve system time in requested format
 * @param1[in]	: requested format conversion info
 * @return		: string;
 */
string currentDateTimeUtc(const char *fmt)
{
    char timeStringBuffer[128] = {0};
    time_t rawTime = time(0);
    struct tm *gmt = gmtime(&rawTime);

    if (fmt) {
        strftime(timeStringBuffer, sizeof(timeStringBuffer), fmt, gmt);
    } else {
        strftime(timeStringBuffer, sizeof(timeStringBuffer),
                "%a %B %e %I:%M:%S %Z %Y", gmt);
    }

    std::string utcDateTime(timeStringBuffer);
    return utcDateTime;
}

/***
 * @brief	: To construct url encoded from string passed
 * @param1[in]	: string; url to be encoded
 * @return		: string; encoded url
 */
std::string url_encode(std::string urlIn)
{
    std::string retval = "";
    CURL *c_url = NULL;

    if (!urlIn.length())
        return retval;

    c_url = curl_easy_init();
    if (c_url) {
        char *encoded = curl_easy_escape(c_url, urlIn.c_str(), urlIn.length());
        retval = encoded;
        curl_free(encoded);
        curl_easy_cleanup(c_url);
    }
    return retval;
}

std::string urlEncodeField(CURL *curl_handle, std::string &data)
{
    std::string encString = "";

    if (curl_handle) {
        char* encoded = curl_easy_escape(curl_handle, data.c_str(), data.length());
        encString = encoded;
        curl_free(encoded);
    }
    return encString;
}

/**
 * @brief : curl write handler
 */
size_t writeCurlResponse(void *ptr, size_t size, size_t nmemb, std::string stream)
{
    size_t realsize = size * nmemb;
    std::string temp(static_cast<const char*>(ptr), realsize);
    stream.append(temp);
    return realsize;
}

/***
 * @brief  : extract mac address value to each key
 * @param1[in] : the entire string from which makc addresses can be extracted
 * @param2[in] : Key to each mac address
 * @param2[out]: mac address value to each key
 */
void findMacInString(std::string totalStr, std::string macId, std::string& mac)
{
    const std::regex re("^([0-9A-F]{2}[:]){5}([0-9A-F]{2})$");
    std::size_t found = totalStr.find(macId);
    mac = totalStr.substr(found + macId.length(), 17);
    std::string defMac = "00:00:00:00:00:00";
    if (!std::regex_match(mac, re)) {
        mac = defMac;
    }
}

/**
 * @brief Used to create/remove XREConnection Retention status file.
 * @return true if status file updation is success; else false.
 */
uint32_t enableXREConnectionRetentionHelper(bool enable)
{
    uint32_t result = SysSrv_Unexpected;

    if (enable) {
        if (!Utils::fileExists(RECEIVER_STANDBY_PREFS)) {
            FILE *fp = NULL;
            if ((fp = fopen(RECEIVER_STANDBY_PREFS, "w"))) {
                fclose(fp);
                result = SysSrv_OK;
            } else {
                return SysSrv_FileAccessFailed;
            }
        } else {
            result=SysSrv_OK;
            
        }
    } else {
        if (Utils::fileExists(RECEIVER_STANDBY_PREFS)) {
            (void)remove(RECEIVER_STANDBY_PREFS);
            result = SysSrv_OK ;
        } else {
         result = SysSrv_OK;
            
        }
    }
    return result;
}

std::string stringTodate(char *pBuffer)
{
    std::string str = "";
    struct tm result;

    if (strptime(pBuffer, "%Y-%m-%d %H:%M:%S", &result) == NULL) {
        return str;
    } else {
        char tempBuff[128] = {'\0'};

        strftime(tempBuff, sizeof(tempBuff), "%a %d %b %Y %H:%M:%S UTC", &result);
        str = tempBuff;
    }
    return str;
}

/**
 * @brief Used to used to remove characters from string.
 * @param1[in] : The string that has to striped of the given characters
 * @param2[in] : char pointer too character arrya that contains all the
 * 				 characters to be removed
 * @param2[out]: String striped of given characters
 */
void removeCharsFromString(string &str, const char *charsToRemove)
{
    for ( unsigned int i = 0; i < strlen(charsToRemove); ++i )
    {
        str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
    }
}

/**
 * @brief : Curl API assistance function
 */
size_t curl_write(void *ptr, size_t size, size_t nmemb, void *stream)
{
    struct write_result *result = (struct write_result *)stream;
    /* Will we overflow on this write? */
    if (result->pos + size * nmemb >= CURL_BUFFER_SIZE - 1) {
        return -1;
    }
    /* Copy curl's stream buffer into our own buffer */
    memcpy(result->data + result->pos, ptr, size * nmemb);
    /* Advance the position */
    result->pos += size * nmemb;

    return size * nmemb;
}

/* Utility API for parsing the  DCM/Device properties file */
bool parseConfigFile(const char* filename, string findkey, string &value)
{
    vector<std::string> lines;
    bool found=false;
    (void)getFileContent(filename,lines);
    for (vector<std::string>::const_iterator i = lines.begin();
            i != lines.end(); ++i){
        string line = *i;
        size_t eq = line.find_first_of("=");
        if (std::string::npos != eq) {
            std::string key = line.substr(0, eq);
            if (key == findkey) {
                value = line.substr(eq + 1);
                found=true;
                break;
            }
        }
    }

    if(found){
        return true;
    }
    else{
        return false;
    }
}
