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

namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(FrameRateImplementation, 1, 0);
        FrameRateImplementation* FrameRateImplementation::_instance = nullptr;

        FrameRateImplementation::FrameRateImplementation()
            : _adminLock()
        {
            FrameRateImplementation::_instance = this;
            InitializeIARM();
        }

        FrameRateImplementation::~FrameRateImplementation()
        {
            DeinitializeIARM();
        }

        // IARM EventHandler
        static void _frameEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

        void FrameRateImplementation::InitializeIARM()
        {
            if (Utils::IARM::init())
            {
                IARM_Result_t res;
                IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, _frameEventHandler));
                IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, _frameEventHandler));
            }
        }

        void FrameRateImplementation::DeinitializeIARM()
        {
            if (Utils::IARM::isConnected())
            {
                IARM_Result_t res;
                IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, _frameEventHandler));
                IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, _frameEventHandler));
            }
        }

        void FrameRateImplementation::dispatchOnDisplayFrameRateChangingEvent(const string& displayFrameRate)
        {
            std::list<Exchange::IFrameRate::INotification*>::const_iterator index(_framerateNotification.begin());
            while (index != _framerateNotification.end())
            {
                (*index)->OnDisplayFrameRateChanging(displayFrameRate);
                ++index;
            }
        }

        void FrameRateImplementation::dispatchOnDisplayFrameRateChangedEvent(const string& displayFrameRate)
        {
            std::list<Exchange::IFrameRate::INotification*>::const_iterator index(_framerateNotification.begin());
            while (index != _framerateNotification.end())
            {
                (*index)->OnDisplayFrameRateChanged(displayFrameRate);
                ++index;
            }
        }

        void FrameRateImplementation::DispatchDSMGRDisplayFramerateChangeEvent(Event event, const JsonValue params)
        {
            _adminLock.Lock();
            switch (event)
            {
                case  DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE:
                    dispatchOnDisplayFrameRateChangingEvent(params.String());
                    break;

                case  DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE:
                    dispatchOnDisplayFrameRateChangedEvent(params.String());
                    break;
            }
            _adminLock.Unlock();
        }

        void _frameEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            switch (eventId)
            {
                case  IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE:
                    {
                        char dispFrameRate[32] = {0};
                        IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                        strncpy(dispFrameRate, eventData->data.DisplayFrameRateChange.framerate, sizeof(dispFrameRate));
                        dispFrameRate[sizeof(dispFrameRate) - 1] = '\0';

                        Core::IWorkerPool::Instance().Submit(FrameRateImplementation::Job::Create(FrameRateImplementation::_instance,
                                    FrameRateImplementation::DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE,
                                    dispFrameRate));
                    }
                    break;

                case  IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE:
                    {
                        char dispFrameRate[32] = {0};
                        IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                        strncpy(dispFrameRate, eventData->data.DisplayFrameRateChange.framerate, sizeof(dispFrameRate));
                        dispFrameRate[sizeof(dispFrameRate) - 1] = '\0';
                        Core::IWorkerPool::Instance().Submit(FrameRateImplementation::Job::Create(FrameRateImplementation::_instance,
                                    FrameRateImplementation::DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE,
                                    dispFrameRate));
                    }
                    break;
            }
        }

        Core::hresult FrameRateImplementation::Register(Exchange::IFrameRate::INotification *notification)
        {
            ASSERT (nullptr != notification);
            _adminLock.Lock();

            // Make sure we can't register the same notification callback multiple times
            if (std::find(_framerateNotification.begin(), _framerateNotification.end(), notification) == _framerateNotification.end())
            {
                _framerateNotification.push_back(notification);
                notification->AddRef();
            }
            else
            {
                LOGERR("same notification is registered already");
            }

            _adminLock.Unlock();
            return Core::ERROR_NONE;
        }

        Core::hresult FrameRateImplementation::Unregister(Exchange::IFrameRate::INotification *notification)
        {
            Core::hresult status = Core::ERROR_GENERAL;
            ASSERT (nullptr != notification);
            _adminLock.Lock();

            // we just unregister one notification once
            auto itr = std::find(_framerateNotification.begin(), _framerateNotification.end(), notification);
            if (itr != _framerateNotification.end())
            {
                (*itr)->Release();
                _framerateNotification.erase(itr);
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("_framerateNotification not found");
            }

            _adminLock.Unlock();
            return status;
        }

        Core::hresult FrameRateImplementation::SetCollectionFrequency(int frequency, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            LOGINFO();
            bool retValue = false;

            try
            {
                if (frequency >= 0)
                {
                    int fpsFrequencyInMilliseconds = DEFAULT_FPS_COLLECTION_TIME_IN_MILLISECONDS;
                    fpsFrequencyInMilliseconds = frequency;

                    // make sure min freq is 100 and not less than that.
                    if (fpsFrequencyInMilliseconds >= 100)
                    {
                        m_fpsCollectionFrequencyInMs = fpsFrequencyInMilliseconds;
                        retValue = true;
                    }
                    else
                    {
                        LOGWARN("Minimum FrameRate is 100.");
                    }
                }
                else
                {
                    LOGWARN("Please enter valid FrameRate Parameter name.");
                }
            }
            catch (...)
            {
                LOGERR("Please enter valid FrameRate value");
            }
            success = retValue;
            return (retValue ? Core::ERROR_NONE : Core::ERROR_GENERAL);
        }

        Core::hresult FrameRateImplementation::StartFpsCollection(bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            LOGINFO();

            if (m_fpsCollectionInProgress)
            {
                success = false;
            }
            if (m_reportFpsTimer.isActive())
            {
                m_reportFpsTimer.stop();
            }

            m_minFpsValue = DEFAULT_MIN_FPS_VALUE;
            m_maxFpsValue = DEFAULT_MAX_FPS_VALUE;
            m_totalFpsValues = 0;
            m_numberOfFpsUpdates = 0;
            m_fpsCollectionInProgress = true;
            int fpsCollectionFrequency = m_fpsCollectionFrequencyInMs;

            if (fpsCollectionFrequency < MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS)
            {
                fpsCollectionFrequency = MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS;
            }
            m_reportFpsTimer.start(fpsCollectionFrequency);
            enableFpsCollection();
            success = true;
            return Core::ERROR_NONE;
        }

        Core::hresult FrameRateImplementation::StopFpsCollection(bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            LOGINFO();

            if (m_reportFpsTimer.isActive())
            {
                m_reportFpsTimer.stop();
            }
            if (m_fpsCollectionInProgress)
            {
                m_fpsCollectionInProgress = false;
                int averageFps = -1;
                int minFps = -1;
                int maxFps = -1;
                if (m_numberOfFpsUpdates > 0)
                {
                    averageFps = (m_totalFpsValues / m_numberOfFpsUpdates);
                    minFps = m_minFpsValue;
                    maxFps = m_maxFpsValue;
                    fpsCollectionUpdate(averageFps, minFps, maxFps);
                }
                disableFpsCollection();
            }
            success = true;
            return Core::ERROR_NONE;
        }

        Core::hresult FrameRateImplementation::UpdateFps(int newFpsValue, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            LOGINFO();

            if (newFpsValue > m_maxFpsValue)
            {
                m_maxFpsValue = newFpsValue;
            }
            if (newFpsValue < m_minFpsValue)
            {
                m_minFpsValue = newFpsValue;
            }
            m_totalFpsValues += newFpsValue;
            m_numberOfFpsUpdates++;
            m_lastFpsValue = newFpsValue;

            success = true;
            return Core::ERROR_NONE;
        }

        Core::hresult FrameRateImplementation::SetFrmMode(int frmmode, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            LOGINFO();

            try
            {
                device::VideoDevice& device = device::Host::getInstance().getVideoDevices().at(0);
                device.setFRFMode(frmmode);
                success = true;
                return Core::ERROR_NONE;
            }
            catch (const device::Exception& err)
            {
                LOGERR("Failed to set frame mode: %s", err.what());
                success = false;
            }
            return Core::ERROR_GENERAL;
        }

        Core::hresult FrameRateImplementation::GetFrmMode(int &frmmode , bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            LOGINFO();

            success = true;
            try
            {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.getFRFMode(&frmmode);
                return Core::ERROR_NONE;
            }
            catch(const device::Exception& err)
            {
                LOGERR("Failed to get frame mode: %s", err.what());
                success = false;
            }
            return Core::ERROR_GENERAL;
        }

        Core::hresult FrameRateImplementation::SetDisplayFrameRate(const string& framerate, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            LOGINFO();
            string sFramerate = framerate;
            success = true;

            try
            {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.setDisplayframerate(sFramerate.c_str());
				return Core::ERROR_NONE;
            }
            catch (const device::Exception& err)
            {
                LOGERR("Failed to set display frame rate: %s", err.what());
                success = false;
            }
            return Core::ERROR_GENERAL;
        }

        Core::hresult FrameRateImplementation::GetDisplayFrameRate(string& framerate, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            LOGINFO();
            char sFramerate[32] = {0};
            success = true;

            try
            {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.getCurrentDisframerate(sFramerate);
                framerate = std::string(sFramerate);
                LOGINFO("Display Frame Rate: %s", framerate.c_str());
                return Core::ERROR_NONE;
            }
            catch (const device::Exception& err)
            {
                LOGERR("Failed to get display frame rate: %s", err.what());
                success = false;
            }

            return Core::ERROR_GENERAL;
        }

        /**
         * @brief This function is used to get the amount of collection interval per milliseconds.
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

            if (m_numberOfFpsUpdates > 0)
            {
                averageFps = (m_totalFpsValues / m_numberOfFpsUpdates);
                minFps = m_minFpsValue;
                maxFps = m_maxFpsValue;
            }

            fpsCollectionUpdate(averageFps, minFps, maxFps);

            if (m_lastFpsValue >= 0)
            {
                // store the last fps value just in case there are no updates
                m_minFpsValue = m_lastFpsValue;
                m_maxFpsValue = m_lastFpsValue;
                m_totalFpsValues = m_lastFpsValue;
                m_numberOfFpsUpdates = 1;
            }
            else
            {
                m_minFpsValue = DEFAULT_MIN_FPS_VALUE;
                m_maxFpsValue = DEFAULT_MAX_FPS_VALUE;
                m_totalFpsValues = 0;
                m_numberOfFpsUpdates = 0;
            }
        }

        void FrameRateImplementation::FrameRatePreChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            char dispFrameRate[32] = {0};
            if (strcmp(owner, IARM_BUS_DSMGR_NAME) == 0)
            {
                switch (eventId)
                {
                    case IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE:
                        IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                        strncpy(dispFrameRate,eventData->data.DisplayFrameRateChange.framerate, sizeof(dispFrameRate));
                        dispFrameRate[sizeof(dispFrameRate) - 1] = '\0';
                        break;
                }
            }

            if (FrameRateImplementation::_instance)
            {
                FrameRateImplementation::_instance->frameRatePreChange(dispFrameRate);
            }
            else
            {
                LOGERR("FrameRateImplementation::_instance is NULL");
            }
        }

        void FrameRateImplementation::frameRatePreChange(char *displayFrameRate)
        {
            string status = std::string(displayFrameRate);
        }

        void FrameRateImplementation::FrameRatePostChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            char dispFrameRate[32] = {0};
            if (strcmp(owner, IARM_BUS_DSMGR_NAME) == 0)
            {
                switch (eventId)
                {
                    case IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE:
                        IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                        strncpy(dispFrameRate,eventData->data.DisplayFrameRateChange.framerate, sizeof(dispFrameRate));
                        dispFrameRate[sizeof(dispFrameRate) - 1] = '\0';
                        break;
                }
            }

            if (FrameRateImplementation::_instance)
            {
                FrameRateImplementation::_instance->frameRatePostChange(dispFrameRate);
            }
            else
            {
                LOGERR("FrameRateImplementation::_instance is NULL");
            }
        }

        void FrameRateImplementation::frameRatePostChange(char *displayFrameRate)
        {
            string status = std::string(displayFrameRate);
        }
    } // namespace Plugin
} // namespace WPEFramework
