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
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <iomanip>
#include <sys/prctl.h>
#include <mutex>

#include "FrameRateImplementation.h"
#include "host.hpp"
#include "exception.hpp"
#include "dsMgr.h"

#include "UtilsJsonRpc.h"
#include "UtilsIarm.h"
#include "UtilsProcess.h"

//Defines
#define DEFAULT_FPS_COLLECTION_TIME_IN_MILLISECONDS 10000
#define MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS 100
#define DEFAULT_MIN_FPS_VALUE 60
#define DEFAULT_MAX_FPS_VALUE -1

// TODO: remove this
#define registerMethod(...) for (uint8_t i = 1; GetHandler(i); i++) GetHandler(i)->Register<JsonObject, JsonObject>(__VA_ARGS__)

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 5

namespace WPEFramework
{
    namespace Plugin
    {
        static void _frameEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

        SERVICE_REGISTRATION(FrameRateImplementation, 1, 0);

        FrameRateImplementation* FrameRateImplementation::_instance = nullptr;

        FrameRateImplementation::FrameRateImplementation() {
            FrameRateImplementation::_instance = this;
            InitializeIARM();
        }

        FrameRateImplementation::~FrameRateImplementation()
        {
            DeinitializeIARM();
        }

        void FrameRateImplementation::InitializeIARM()
        {
            LOGWARN("FrameRateImplementation::InitializeIARM");
            if (Utils::IARM::init()) {
                IARM_Result_t res;
                IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, _frameEventHandler));
                IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, _frameEventHandler));
            }
        }

        void FrameRateImplementation::DeinitializeIARM()
        {
            if (Utils::IARM::isConnected()) {
                IARM_Result_t res;
                IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, _frameEventHandler));
                IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, _frameEventHandler));
            }
        }

        void FrameRateImplementation::dispatchOnDisplayFrameRateChangingEvent(const string& displayFrameRate)
        {
            std::list<Exchange::IFrameRate::INotification*>::const_iterator index(_framerateNotification.begin());
            while (index != _framerateNotification.end()) {
                (*index)->OnDisplayFrameRateChanging(displayFrameRate);
                ++index;
            }
        }

        void FrameRateImplementation::dispatchOnDisplayFrameRateChangedEvent(const string& displayFrameRate)
        {
            std::list<Exchange::IFrameRate::INotification*>::const_iterator index(_framerateNotification.begin());
            while (index != _framerateNotification.end()) {
                (*index)->OnDisplayFrameRateChanged(displayFrameRate);
                ++index;
            }
        }

        void FrameRateImplementation::Dispatch(Event event, const JsonValue params)
        {
            _adminLock.Lock();
            switch (event) {
                case DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE:
                    dispatchOnDisplayFrameRateChangingEvent(params.String());
                	break;
                case DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE:
                    dispatchOnDisplayFrameRateChangedEvent(params.String());
                	break;
				default:
					LOGERR("FrameRateImplementation::Dispatch: Unknown event: %d", event);
					break;
            }
            _adminLock.Unlock();
        }

        void _frameEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if ((nullptr == owner) || (nullptr == data)) {
                LOGERR("_frameEventHandler: received null parameter; discarding.");
                return;
            }
            switch (eventId) {
                case IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE:
                    {
                        char dispFrameRate[20] ={0};
                        IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                        strncpy(dispFrameRate, eventData->data.DisplayFrameRateChange.framerate, sizeof(dispFrameRate));
                        dispFrameRate[sizeof(dispFrameRate) - 1] = '\0';

                        Core::IWorkerPool::Instance().Submit(FrameRateImplementation::Job::Create(FrameRateImplementation::_instance,
                                    FrameRateImplementation::DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE,
                                    dispFrameRate));

                        LOGINFO("IARM Event triggered for Display framerate prechange: %s", dispFrameRate);
                    }
                    break;

                case IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE:
                    {
                        char dispFrameRate[20] ={0};
                        IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                        strncpy(dispFrameRate,eventData->data.DisplayFrameRateChange.framerate, sizeof(dispFrameRate));
                        dispFrameRate[sizeof(dispFrameRate) - 1] = '\0';
                        Core::IWorkerPool::Instance().Submit(FrameRateImplementation::Job::Create(FrameRateImplementation::_instance,
                                    FrameRateImplementation::DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE,
                                    dispFrameRate));

                        LOGINFO("IARM Event triggered for Display framerate postchange: %s", dispFrameRate);
                    }
                    break;
				default:
					LOGERR("_frameEventHandler: Unknown event: %d", eventId);
					break;
            }
        }

        uint32_t FrameRateImplementation::Register(Exchange::IFrameRate::INotification *notification)
        {
            ASSERT (nullptr != notification);
            _adminLock.Lock();

            // Make sure we can't register the same notification callback multiple times
            if (std::find(_framerateNotification.begin(), _framerateNotification.end(), notification) == _framerateNotification.end()) {
                _framerateNotification.push_back(notification);
                notification->AddRef();
            } else {
                LOGERR("same notification is registered already");
            }

            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }

        uint32_t FrameRateImplementation::Unregister(Exchange::IFrameRate::INotification *notification)
        {
            uint32_t status = Core::ERROR_GENERAL;

            ASSERT (nullptr != notification);

            _adminLock.Lock();

            // we just unregister one notification once
            auto itr = std::find(_framerateNotification.begin(), _framerateNotification.end(), notification);
            if (itr != _framerateNotification.end()) {
                (*itr)->Release();
                _framerateNotification.erase(itr);
                status = Core::ERROR_NONE;
            } else {
                LOGERR("notification not found");
            }

            _adminLock.Unlock();

            return status;
        }

        uint32_t FrameRateImplementation::SetCollectionFrequency(int frequency, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            LOGINFO();

            bool retValue = false;
            try {
                if (frequency >= 0) {
                    int fpsFrequencyInMilliseconds = DEFAULT_FPS_COLLECTION_TIME_IN_MILLISECONDS;
                    fpsFrequencyInMilliseconds = frequency;
					 // make sure min freq is 100 and not less than that.
                    if (fpsFrequencyInMilliseconds >= 100) {
                        m_fpsCollectionFrequencyInMs = fpsFrequencyInMilliseconds;
                        retValue = true;
                    } else {
                        LOGWARN("Minimum FrameRate is 100.");
                        retValue = false;
                    }
                } else {
                    LOGWARN("Please enter valid FrameRate Parameter name.");
                    retValue = false;
                }
            } catch(...) {
                LOGERR("Please enter valid FrameRate value");
                retValue = false;
            }
            success = retValue;
            return Core::ERROR_GENERAL;
        }

        uint32_t FrameRateImplementation::StartFpsCollection(bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            LOGINFO();
            if (m_fpsCollectionInProgress) {
                success = false;
            }
            if (m_reportFpsTimer.isActive()) {
                m_reportFpsTimer.stop();
            }
            m_minFpsValue = DEFAULT_MIN_FPS_VALUE;
            m_maxFpsValue = DEFAULT_MAX_FPS_VALUE;
            m_totalFpsValues = 0;
            m_numberOfFpsUpdates = 0;
            m_fpsCollectionInProgress = true;
            int fpsCollectionFrequency = m_fpsCollectionFrequencyInMs;
            if (fpsCollectionFrequency < MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS) {
                fpsCollectionFrequency = MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS;
            }
            m_reportFpsTimer.start(fpsCollectionFrequency);
            enableFpsCollection();
            success = true;
            return Core::ERROR_GENERAL;
        }

        uint32_t FrameRateImplementation::StopFpsCollection(bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFO();

            if (m_reportFpsTimer.isActive()) {
                m_reportFpsTimer.stop();
            }
            if (m_fpsCollectionInProgress) {
                m_fpsCollectionInProgress = false;
                int averageFps = -1;
                int minFps = -1;
                int maxFps = -1;
                if (m_numberOfFpsUpdates > 0) {
                    averageFps = (m_totalFpsValues / m_numberOfFpsUpdates);
                    minFps = m_minFpsValue;
                    maxFps = m_maxFpsValue;
                    fpsCollectionUpdate(averageFps, minFps, maxFps);
                }
                disableFpsCollection();
            }
            success = true;
            return Core::ERROR_GENERAL;
        }

        uint32_t FrameRateImplementation::UpdateFps(int newFpsValue, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFO();

            if (newFpsValue > m_maxFpsValue) {
                m_maxFpsValue = newFpsValue;
            }
            if (newFpsValue < m_minFpsValue) {
                m_minFpsValue = newFpsValue;
            }
            m_totalFpsValues += newFpsValue;
            m_numberOfFpsUpdates++;
            m_lastFpsValue = newFpsValue;

            success = true;
            return Core::ERROR_GENERAL;
        }

        uint32_t FrameRateImplementation::SetFrmMode(int frmmode, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFO();
            int frfmode = 0;
            try {
                frfmode = frmmode;
            } catch (const device::Exception& err) {
                success = false;
                return Core::ERROR_GENERAL;
            }
            success = true;

            try {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.setFRFMode(frfmode);
            } catch (const device::Exception& err) {
                //LOG_DEVICE_EXCEPTION1(sPortId);
                success = false;
            }
            return Core::ERROR_GENERAL;
        }

        uint32_t FrameRateImplementation::GetFrmMode(int &frmmode, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFO();
            success = true;
            try {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.getFRFMode(&frmmode);
            } catch (const device::Exception& err) {
                //LOG_DEVICE_EXCEPTION0();
                success = false;
            }
            return Core::ERROR_GENERAL;
        }

        uint32_t FrameRateImplementation::SetDisplayFrameRate(const string& framerate, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFO();

            string sFramerate = framerate;

            success = true;
            try {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.setDisplayframerate(sFramerate.c_str());
            } catch (const device::Exception& err) {
                //LOG_DEVICE_EXCEPTION1(sFramerate);
                success = false;
            }
            return Core::ERROR_GENERAL;
        }

        uint32_t FrameRateImplementation::GetDisplayFrameRate(string& framerate, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFO();
            char sFramerate[20] = {0};
            success = true;
            try {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.getCurrentDisframerate(sFramerate);
            } catch (const device::Exception& err) {
                //LOG_DEVICE_EXCEPTION1(std::string(sFramerate));
                success = false;
            }

            framerate = std::string(sFramerate);
            return Core::ERROR_GENERAL;
        }

        /**
         * @brief This function is used to get the amount of collection interval per milliseconds.
         *
         * @return Integer value of Amount of milliseconds per collection interval .
         */
        int FrameRateImplementation::getCollectionFrequency()
        {
            return m_fpsCollectionFrequencyInMs;
        }

        void FrameRateImplementation::fpsCollectionUpdate(int averageFps, int minFps, int maxFps)
        {
            JsonObject params;
            params["average"] = averageFps;
            params["min"] = minFps;
            params["max"] = maxFps;
        }

        void FrameRateImplementation::onReportFpsTimer()
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            int averageFps = -1;
            int minFps = -1;
            int maxFps = -1;
            if (m_numberOfFpsUpdates > 0) {
                averageFps = (m_totalFpsValues / m_numberOfFpsUpdates);
                minFps = m_minFpsValue;
                maxFps = m_maxFpsValue;
            }
            fpsCollectionUpdate(averageFps, minFps, maxFps);
            if (m_lastFpsValue >= 0) {
                // store the last fps value just in case there are no updates
                m_minFpsValue = m_lastFpsValue;
                m_maxFpsValue = m_lastFpsValue;
                m_totalFpsValues = m_lastFpsValue;
                m_numberOfFpsUpdates = 1;
            } else {
                m_minFpsValue = DEFAULT_MIN_FPS_VALUE;
                m_maxFpsValue = DEFAULT_MAX_FPS_VALUE;
                m_totalFpsValues = 0;
                m_numberOfFpsUpdates = 0;
            }
        }

        void FrameRateImplementation::FrameRatePreChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if ((nullptr == owner) || (nullptr == data)) {
                LOGERR("FrameRateImplementation::FrameRatePreChange: received null parameter; discarding.");
                return;
            }

            char dispFrameRate[20] = {0};
            if (strcmp(owner, IARM_BUS_DSMGR_NAME) == 0) {
                switch (eventId) {
                    case IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE:
                        IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                        strncpy(dispFrameRate, eventData->data.DisplayFrameRateChange.framerate, sizeof(dispFrameRate));
                        dispFrameRate[sizeof(dispFrameRate) - 1] = '\0';
                        break;
                }
            }

            if (FrameRateImplementation::_instance) {
                FrameRateImplementation::_instance->frameRatePreChange(dispFrameRate);
            }
        }

        void FrameRateImplementation::frameRatePreChange(char *displayFrameRate)
        {
            if (nullptr == displayFrameRate) {
                LOGERR("FrameRateImplementation::frameRatePreChange: received null parameter; discarding.");
                return;
            }
            string status = std::string(displayFrameRate);
        }

        void FrameRateImplementation::FrameRatePostChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if ((nullptr == owner) || (nullptr == data)) {
                LOGERR("FrameRateImplementation::FrameRatePostChange: received null parameter; discarding.");
                return;
            }
            char dispFrameRate[20] = {0};
            if (strcmp(owner, IARM_BUS_DSMGR_NAME) == 0) {
                switch (eventId) {
                    case IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE:
                        IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                        strncpy(dispFrameRate, eventData->data.DisplayFrameRateChange.framerate, sizeof(dispFrameRate));
                        dispFrameRate[sizeof(dispFrameRate) - 1] = '\0';
                        break;
                }
            }

            if (FrameRateImplementation::_instance) {
                FrameRateImplementation::_instance->frameRatePostChange(dispFrameRate);
            }
        }

        void FrameRateImplementation::frameRatePostChange(char *displayFrameRate)
        {
            if (nullptr == displayFrameRate) {
                LOGERR("FrameRateImplementation::frameRatePostChange: received null parameter; discarding.");
                return;
            }
            string status = std::string(displayFrameRate);
        }
    } // namespace Plugin
} // namespace WPEFramework
