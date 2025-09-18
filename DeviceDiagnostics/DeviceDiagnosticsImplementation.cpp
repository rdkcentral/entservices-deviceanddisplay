/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
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
**/

#include "DeviceDiagnosticsImplementation.h"
#include <curl/curl.h>
#include <time.h>
#include <fstream>

#include "UtilsJsonRpc.h"

#define MILESTONES_LOG_FILE                     "/opt/logs/rdk_milestones.log"


/***
 * @brief  : Used to read file contents into a vector
 * @param1[in] : Complete file name with path
 * @param2[in] : Destination list buffer to be filled with file contents
 * @return : <bool>; TRUE if operation success; else FALSE.
 */
bool getFileContent(std::string fileName, std::list<std::string> & listOfStrs);

namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(DeviceDiagnosticsImplementation, 1, 0);
        DeviceDiagnosticsImplementation* DeviceDiagnosticsImplementation::_instance = nullptr;
    
        const int curlTimeoutInSeconds = 30;
        static const char *decoderStatusStr[] = {
            "IDLE",
            "PAUSED",
            "ACTIVE",
            NULL
        };

        static size_t writeCurlResponse(void *ptr, size_t size, size_t nmemb, std::string stream)
        {
            size_t realsize = size * nmemb;
            std::string temp(static_cast<const char*>(ptr), realsize);
            stream.append(temp);
            return realsize;
        }

        DeviceDiagnosticsImplementation::DeviceDiagnosticsImplementation() : _adminLock() , _service(nullptr)
        {
            LOGINFO("Create DeviceDiagnosticsImplementation Instance");

            DeviceDiagnosticsImplementation::_instance = this;

#ifdef ENABLE_ERM

            if ((m_EssRMgr = EssRMgrCreate()) == NULL)
            {
                LOGERR("EssRMgrCreate() failed");
                return;
            }

            m_pollThreadRun = 1;
            m_AVPollThread = std::thread(AVPollThread, this);
#else
            LOGWARN("ENABLE_ERM is not defined, decoder status will "
                    "always be reported as IDLE");
#endif
        }

        DeviceDiagnosticsImplementation::~DeviceDiagnosticsImplementation()
        {
#ifdef ENABLE_ERM
            m_AVDecoderStatusLock.lock();
            m_pollThreadRun = 0;
            m_AVDecoderStatusLock.unlock();
            m_avDecoderStatusCv.notify_one();
			if (m_AVPollThread.joinable())
			{
                m_AVPollThread.join();
			}
            EssRMgrDestroy(m_EssRMgr);
#endif
            DeviceDiagnosticsImplementation::_instance = nullptr;
            _service = nullptr;
        }

        Core::hresult DeviceDiagnosticsImplementation::Register(Exchange::IDeviceDiagnostics::INotification *notification)
        {
            ASSERT (nullptr != notification);

            _adminLock.Lock();

            // Make sure we can't register the same notification callback multiple times
            if (std::find(_deviceDiagnosticsNotification.begin(), _deviceDiagnosticsNotification.end(), notification) == _deviceDiagnosticsNotification.end())
            {
                _deviceDiagnosticsNotification.push_back(notification);
                notification->AddRef();
            }
            else
            {
                LOGERR("same notification is registered already");
            }

            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }

        Core::hresult DeviceDiagnosticsImplementation::Unregister(Exchange::IDeviceDiagnostics::INotification *notification )
        {
            Core::hresult status = Core::ERROR_GENERAL;

            ASSERT (nullptr != notification);

            _adminLock.Lock();

            // we just unregister one notification once
            auto itr = std::find(_deviceDiagnosticsNotification.begin(), _deviceDiagnosticsNotification.end(), notification);
            if (itr != _deviceDiagnosticsNotification.end())
            {
                (*itr)->Release();
                _deviceDiagnosticsNotification.erase(itr);
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("notification not found");
            }

            _adminLock.Unlock();

            return status;
        }

        void DeviceDiagnosticsImplementation::dispatchEvent(Event event, const JsonValue &params)
        {
            Core::IWorkerPool::Instance().Submit(Job::Create(this, event, params));
        }

        void DeviceDiagnosticsImplementation::Dispatch(Event event, const JsonValue params)
        {
            _adminLock.Lock();
        
            std::list<Exchange::IDeviceDiagnostics::INotification*>::const_iterator index(_deviceDiagnosticsNotification.begin());
        
            switch(event)
            {
                case ON_AVDECODER_STATUSCHANGED:
                    while (index != _deviceDiagnosticsNotification.end()) 
                    {
                        (*index)->OnAVDecoderStatusChanged(params.String());
                        ++index;
                    }
                    break;
 
                default:
                    LOGWARN("Event[%u] not handled", event);
                    break;
            }
            _adminLock.Unlock();
        }
    
        /* retrieves most active decoder status from ERM library,
         * this library keeps state of all decoders and will give
         * us only the most active status of any decoder */
        int DeviceDiagnosticsImplementation::getMostActiveDecoderStatus()
        {
            int status = 0;
#ifdef ENABLE_ERM
            EssRMgrGetAVState(m_EssRMgr, &status);
#endif
            return status;
        }

        /* periodically polls ERM library for changes in most
         * active decoder and send thunder event when decoder
         * status changes. Needs to be done via poll and separate
         * thread because ERM doesn't support events. */
#ifdef ENABLE_ERM
        void *DeviceDiagnosticsImplementation::AVPollThread(void *arg)
        {
            int lastStatus = EssRMgrRes_idle;
            int status;
            int timeoutInSec = AVDECODERSTATUS_RETRY_INTERVAL;
            DeviceDiagnosticsImplementation* t = DeviceDiagnosticsImplementation::_instance;

            LOGINFO("AVPollThread started");
            for (;;)
            {
                std::unique_lock<std::mutex> lock(t->m_AVDecoderStatusLock);
                if (t->m_avDecoderStatusCv.wait_for(lock, std::chrono::seconds(timeoutInSec)) != std::cv_status::timeout)
                {
                    LOGINFO("Received signal. skipping %d sec interval", timeoutInSec);
                }

                if (t->m_pollThreadRun == 0)
                    break;

                status = t->getMostActiveDecoderStatus();
                lock.unlock();

                if (status == lastStatus)
                    continue;

                lastStatus = status;
                t->onDecoderStatusChange(status);
            }

            return NULL;
        }
#endif

        void DeviceDiagnosticsImplementation::onDecoderStatusChange(int status)
        {
            JsonObject params;
            params["avDecoderStatusChange"] = decoderStatusStr[status];
            dispatchEvent(ON_AVDECODER_STATUSCHANGED, params);
        }

        Core::hresult DeviceDiagnosticsImplementation::GetConfiguration(IStringIterator* const& names, Exchange::IDeviceDiagnostics::IDeviceDiagnosticsParamListIterator*& paramList, bool& success)
        {
	    LOGINFO("");

	    std::string entry;
            JsonObject requestParams;
            JsonArray namePairs;
            std::list<ParamList> deviceDiagnosticsList;

            while (names->Next(entry) == true)
            {
	        JsonObject o;
                o["name"] = entry;
                namePairs.Add(o);
	    }

            requestParams["paramList"] = namePairs;
	    string json;
            requestParams.ToString(json);

            if (0 == getConfig(json, deviceDiagnosticsList))
            {
                paramList = Core::Service<RPC::IteratorType<Exchange::IDeviceDiagnostics::IDeviceDiagnosticsParamListIterator>> \
				::Create<Exchange::IDeviceDiagnostics::IDeviceDiagnosticsParamListIterator>(deviceDiagnosticsList);
                success = true;
                return Core::ERROR_NONE;
            }

            success = false;
            return Core::ERROR_GENERAL;
        }

        Core::hresult DeviceDiagnosticsImplementation::GetMilestones(IStringIterator*& milestones, bool& success)
        {
            uint32_t result = Core::ERROR_NONE;
            bool retAPIStatus = false;
	    std::list<string> list;

            LOGINFO("");

            if (Core::File(string(MILESTONES_LOG_FILE)).Exists())
            {
                retAPIStatus = getFileContent(MILESTONES_LOG_FILE, list);
                if (!retAPIStatus)
                {
                    LOGERR("File access failed");
                    success = false;
                    result = Core::ERROR_GENERAL;
                }
            }
            else 
            {
                LOGERR("Expected file not found");
                success = false;
                result = Core::ERROR_GENERAL;
            }

            if (result == Core::ERROR_NONE)
            {
                milestones = (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(list));
                success = true;
            }

            return result;
        }

        Core::hresult DeviceDiagnosticsImplementation::LogMilestone(const string& marker, bool& success)
        {
	    LOGINFO("");
            if (marker.empty())
            {
                LOGERR("Empty marker' parameter");
                success = false;
                return Core::ERROR_GENERAL;
            }

#ifdef RDK_LOG_MILESTONE
            logMilestone(marker.c_str());
#endif
            success = true;
            return Core::ERROR_NONE; 

        }

        Core::hresult DeviceDiagnosticsImplementation::GetAVDecoderStatus(AvDecoderStatusResult& AVDecoderStatus)
        {
            LOGINFO("");
#ifdef ENABLE_ERM
            m_AVDecoderStatusLock.lock();
            int status = getMostActiveDecoderStatus();
            m_AVDecoderStatusLock.unlock();
            AVDecoderStatus.avDecoderStatus = decoderStatusStr[status];
#else
            AVDecoderStatus.avDecoderStatus = decoderStatusStr[0];
#endif
            return Core::ERROR_NONE;
        }

        int DeviceDiagnosticsImplementation::getConfig(const std::string& postData, std::list<ParamList>& paramListInfo)
        {
            LOGINFO("%s",__FUNCTION__);

            int result = -1;

            long http_code = 0;
            std::string response;
            CURL *curl_handle = NULL;
            CURLcode res = CURLE_OK;
            curl_handle = curl_easy_init();

            LOGINFO("data: %s", postData.c_str());

            if (curl_handle)
            {
                if(curl_easy_setopt(curl_handle, CURLOPT_URL, "http://127.0.0.1:10999") != CURLE_OK)
                    LOGWARN("Failed to set curl option: CURLOPT_URL");
                if(curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, postData.c_str()) != CURLE_OK)
                    LOGWARN("Failed to set curl option: CURLOPT_POSTFIELDS");
                if(curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, postData.size()) != CURLE_OK)
                    LOGWARN("Failed to set curl option: CURLOPT_POSTFIELDSIZE");
                if(curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1) != CURLE_OK) //when redirected, follow the redirections
                    LOGWARN("Failed to set curl option: CURLOPT_FOLLOWLOCATION");
                if(curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writeCurlResponse) != CURLE_OK)
                    LOGWARN("Failed to set curl option: CURLOPT_WRITEFUNCTION");
                if(curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response) != CURLE_OK)
                    LOGWARN("Failed to set curl option: CURLOPT_WRITEDATA");
                if(curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, curlTimeoutInSeconds) != CURLE_OK)
                    LOGWARN("Failed to set curl option: CURLOPT_TIMEOUT");

                res = curl_easy_perform(curl_handle);
                curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);

                LOGWARN("Perfomed curl call : %d http response code: %ld", res, http_code);
                curl_easy_cleanup(curl_handle);
            }
            else
            {
                LOGWARN("Could not perform curl ");
            }

            if (res == CURLE_OK && (http_code == 0 || http_code == 200))
            {
                LOGWARN("curl Response: %s", response.c_str());

                ParamList param;
                std::string::size_type start = 0, end = 0;
                while ((start = response.find("\"name\":", end)) != std::string::npos) 
                {
                    start = response.find("\"", start + 6) + 1;
                    end = response.find("\"", start);
                    param.name = response.substr(start, end - start);

                    start = response.find("\"value\":", end) + 8;
                    end = response.find("}", start);
                    param.value = response.substr(start, end - start);

                    paramListInfo.push_back(param);
                }
                result = 0;
            }
            return result;
        }
    } // namespace Plugin
} // namespace WPEFramework

/***
 * @brief   : Used to read file contents into a list
 * @param1[in]      : Complete file name with path
 * @param2[in]      : Destination list buffer to be filled with file contents
 * @return  : <bool>; TRUE if operation success; else FALSE.
 */
bool getFileContent(std::string fileName, std::list<std::string> & listOfStrs)
{
    bool retStatus = false;
    std::ifstream inFile(fileName.c_str(), std::ios::in);

    if (!inFile.is_open())
        return retStatus;

    std::string line;
    retStatus = true;
    while (std::getline(inFile, line)) {
        if (line.size() > 0) {
            listOfStrs.push_back(line);
        }
    }
    inFile.close();
    return retStatus;
}
