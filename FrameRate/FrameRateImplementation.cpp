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

#include "UtilsIarm.h"
#include "UtilsLogging.h"

//Defines
#define DEFAULT_FPS_COLLECTION_TIME_IN_MILLISECONDS 10000
#define MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS 100
#define DEFAULT_MIN_FPS_VALUE 60
#define DEFAULT_MAX_FPS_VALUE -1

#define ENABLE_DEBUG 1
#ifdef ENABLE_DEBUG
#define DBGINFO(fmt, ...) LOGINFO(fmt, ##__VA_ARGS__)
#else
#define DBGINFO(fmt, ...)
#endif

namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(FrameRateImplementation, 1, 0);
        FrameRateImplementation* FrameRateImplementation::_instance = nullptr;

        FrameRateImplementation::FrameRateImplementation()
            : _adminLock()
              , m_fpsCollectionFrequencyInMs(DEFAULT_FPS_COLLECTION_TIME_IN_MILLISECONDS)
              , m_minFpsValue(DEFAULT_MIN_FPS_VALUE)
              , m_maxFpsValue(DEFAULT_MAX_FPS_VALUE)
              , m_totalFpsValues(0)
              , m_numberOfFpsUpdates(0)
              , m_fpsCollectionInProgress(false)
              , m_lastFpsValue(0)
        {
            FrameRateImplementation::_instance = this;
            InitializeIARM();
            m_reportFpsTimer.connect(std::bind(&FrameRateImplementation::onReportFpsTimer, this));
        }

        FrameRateImplementation::~FrameRateImplementation()
        {
            DeinitializeIARM();
        }

        void FrameRateImplementation::InitializeIARM()
        {
            if (Utils::IARM::init())
            {
                IARM_Result_t res;
                IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, _iarmbusEventHandlerCB));
                IARM_CHECK(IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, _iarmbusEventHandlerCB));
            }
        }

        void FrameRateImplementation::DeinitializeIARM()
        {
            if (Utils::IARM::isConnected())
            {
                IARM_Result_t res;
                IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, _iarmbusEventHandlerCB));
                IARM_CHECK(IARM_Bus_RemoveEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, _iarmbusEventHandlerCB));
            }
        }

        /****************************************** Notifications ***********************************************/

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

        /**
         * @brief This function is used to dispatch 'onFpsEvent' to all registered notifications.
         * @param average - The average frame rate.
         * @param min - The minimum frame rate.
         * @param max - The maximum frame rate.
         * @return void
         */
        void FrameRateImplementation::dispatchOnFpsEvent(int average, int min, int max)
        {
            std::list<Exchange::IFrameRate::INotification*>::const_iterator index(_framerateNotification.begin());
            while (index != _framerateNotification.end())
            {
                (*index)->OnFpsEvent(average, min, max);
                ++index;
            }
            DBGINFO("average = %d, min = %d, max = %d.", average, min, max);
        }

        /**
         * @brief This function is used to dispatch 'onDisplayFrameRateChanging' to all registered notifications.
         * @param displayFrameRate - The display frame rate.
         * @return void
         */
        void FrameRateImplementation::dispatchOnDisplayFrameRateChangingEvent(const string& displayFrameRate)
        {
            std::list<Exchange::IFrameRate::INotification*>::const_iterator index(_framerateNotification.begin());
            while (index != _framerateNotification.end())
            {
                (*index)->OnDisplayFrameRateChanging(displayFrameRate);
                ++index;
            }
            DBGINFO("displayFrameRate = %s.", displayFrameRate.c_str());
        }

        /**
         * @brief This function is used to dispatch 'onDisplayFrameRateChanged' to all registered notifications.
         * @param displayFrameRate - The display frame rate.
         * @return void
         */
        void FrameRateImplementation::dispatchOnDisplayFrameRateChangedEvent(const string& displayFrameRate)
        {
            std::list<Exchange::IFrameRate::INotification*>::const_iterator index(_framerateNotification.begin());
            while (index != _framerateNotification.end())
            {
                (*index)->OnDisplayFrameRateChanged(displayFrameRate);
                ++index;
            }
            DBGINFO("displayFrameRate = %s.", displayFrameRate.c_str());
        }

        /**
         * @brief This function is used to dispatch the display frame rate change event, triggered by worker pool.
         * @param event - The event type (prechange or postchange).
         * @param params - The parameters associated with the event.
         * @return void
         */
        void FrameRateImplementation::DispatchDSMGRDisplayFramerateChangeEvent(Event event, const JsonValue params)
        {
            DBGINFO("event = %d.", event);
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

        /**
         * @brief This function is used to handle the IARM event callback.
         * @param owner - The owner of the event.
         * @param eventId - The ID of the event.
         * @param data - The data associated with the event.
         * @param len - The length of the data.
         * @return void
         */
        void _iarmbusEventHandlerCB(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            switch (eventId)
            {
                case  IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE:
                    {
                        char dispFrameRate[32] = {0};
                        IARM_Bus_DSMgr_EventData_t *eventData = (IARM_Bus_DSMgr_EventData_t *)data;
                        strncpy(dispFrameRate, eventData->data.DisplayFrameRateChange.framerate, sizeof(dispFrameRate));
                        dispFrameRate[sizeof(dispFrameRate) - 1] = '\0';
                        DBGINFO("Display Frame Rate PreChange: %s", dispFrameRate);
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
                        DBGINFO("Display Frame Rate PreChange: %s", dispFrameRate);
                        Core::IWorkerPool::Instance().Submit(FrameRateImplementation::Job::Create(FrameRateImplementation::_instance,
                                    FrameRateImplementation::DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE,
                                    dispFrameRate));
                    }
                    break;
            }
        }

		/********************************************* Methods *****************************************************/

        /**
         * @brief Returns the current display frame rate values.
         * @param framerate - The current display framerate setting (width x height x framerate)
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::GetDisplayFrameRate(string& framerate, bool& success)
        {
            success = false;
            std::lock_guard<std::mutex> guard(m_callMutex);

            try
            {
                device::List<device::VideoDevice> videoDevices = device::Host::getInstance().getVideoDevices();
                if (videoDevices.size() == 0)
                {
                    LOGERR("No video devices available.");
                    return Core::ERROR_GENERAL;
                }

                char sFramerate[32] = {0};
                device::VideoDevice& device = videoDevices.at(0);
                if (!device.getCurrentDisframerate(sFramerate) && sFramerate[0] != '\0')
                {
                    framerate = sFramerate;
                    success = true;
                    return Core::ERROR_NONE;
                }

                LOGERR("getCurrentDisframerate error, DS::ERROR.");
            }
            catch (const device::Exception& err)
            {
                LOGERR("Exception: %s", err.what());
            }
            return Core::ERROR_GENERAL;
        }

        /**
         * @brief Returns the current auto framerate mode.
         * @param autoFRMMode - The current auto framerate mode.
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::GetFrmMode(int &autoFRMMode , bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            try
            {
                device::List<device::VideoDevice> videoDevices = device::Host::getInstance().getVideoDevices();
                if (videoDevices.size() == 0)
                {
                    LOGERR("No video devices available.");
                    return Core::ERROR_GENERAL;
                }
                device::VideoDevice& device = videoDevices.at(0);
                if (!device.getFRFMode(&autoFRMMode))
                {
                    DBGINFO("Frame Mode: %d", autoFRMMode);
                    success = true;
                    return Core::ERROR_NONE;
                }
                LOGERR("getFRFMode failed DS::ERROR.");
            }
            catch(const device::Exception& err)
            {
                LOGERR("Exception: %s", err.what());
            }
            success = false;
            return Core::ERROR_GENERAL;
        }

        /**
         * @brief This function is used to set the FPS data collection interval.
         * @param frequency - The amount of time in milliseconds. Default is 10000ms and min is 100ms.
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::SetCollectionFrequency(int frequency, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            if (frequency < MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS)
            {
                LOGERR("Invalid frequency, minimum is %d ms.", MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS);
                success = false;
                return Core::ERROR_GENERAL;
            }

            m_fpsCollectionFrequencyInMs = frequency;
            DBGINFO("FrameRate collection frequency set to %d milliseconds.", frequency);
            success = true;
            return Core::ERROR_NONE;
        }

        /**
         * @brief Sets the display framerate values.
         * @param framerate - The display frame rate in the format "WIDTHxHEIGHTxFPS".
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::SetDisplayFrameRate(const string& framerate, bool& success)
        {
            // framerate should be of "WIDTHxHEIGHTxFPS" as per DSHAL specification - setDisplayframerate
            // Eg: 1920x1080x60
            // check if we got two 'x' in the string at least.
            success = false;
            if (std::count(framerate.begin(), framerate.end(), 'x') != 2)
            {
                LOGERR("Invalid frame rate format: %s", framerate.c_str());
                return Core::ERROR_GENERAL;
            }
            string sFramerate = framerate;
            std::lock_guard<std::mutex> guard(m_callMutex);

            try
            {
                device::List<device::VideoDevice> videoDevices = device::Host::getInstance().getVideoDevices();
                if (videoDevices.size() == 0)
                {
                    LOGERR("No video devices available.");
                    return Core::ERROR_GENERAL;
                }
                device::VideoDevice& device = videoDevices.at(0);
                if (!device.setDisplayframerate(sFramerate.c_str()))
                {
                    success = true;
                    return Core::ERROR_NONE;
                }
                LOGERR("setDisplayframerate failed, DS::ERROR.");
            }
            catch (const device::Exception& err)
            {
                LOGERR("Exception: %s", err.what());
            }
            return Core::ERROR_GENERAL;
        }

        /**
         * @brief Sets the auto framerate mode.
         * @param frmmode - The frame mode (0 or 1).
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::SetFrmMode(int frmmode, bool& success)
        {
            if (frmmode != 0 && frmmode != 1)
            {
                LOGERR("Invalid frame mode: %d", frmmode);
                success = false;
                return Core::ERROR_GENERAL;
            }

            std::lock_guard<std::mutex> guard(m_callMutex);

            try
            {
                device::List<device::VideoDevice> videoDevices = device::Host::getInstance().getVideoDevices();
                if (videoDevices.size() == 0)
                {
                    LOGERR("No video devices available.");
                    return Core::ERROR_GENERAL;
                }
                device::VideoDevice& device = videoDevices.at(0);
                if (!device.setFRFMode(frmmode))
                {
                    success = true;
                    return Core::ERROR_NONE;
                }
                DBGINFO("Failed to set frame mode DS::ERROR  %d", frmmode);
            }
            catch (const device::Exception& err)
            {
                LOGERR("Failed to set frame mode: %s", err.what());
            }
            success = false;
            return Core::ERROR_GENERAL;
        }

        /**
         * @brief Starts the FPS data collection.
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::StartFpsCollection(bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            if (m_fpsCollectionInProgress)
            {
                DBGINFO("FPS collection is already in progress.");
            }
            if (m_reportFpsTimer.isActive())
            {
                DBGINFO("FPS collection timer is already active, stopping it.");
                m_reportFpsTimer.stop();
            }

            m_minFpsValue = DEFAULT_MIN_FPS_VALUE;
            m_maxFpsValue = DEFAULT_MAX_FPS_VALUE;
            m_totalFpsValues = 0;
            m_numberOfFpsUpdates = 0;
            m_fpsCollectionInProgress = true;
            int fpsCollectionFrequency = m_fpsCollectionFrequencyInMs;
            m_lastFpsValue = -1;
            m_reportFpsTimer.start(fpsCollectionFrequency);
            DBGINFO("FPS collection timer started with frequency %d milliseconds.", fpsCollectionFrequency);
            success = true;
            return Core::ERROR_NONE;
        }

        /**
         * @brief Stops the FPS data collection.
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::StopFpsCollection(bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

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
                    dispatchOnFpsEvent(averageFps, minFps, maxFps);
                }
            }
            success = true;
            return Core::ERROR_NONE;
        }

        /**
         * @brief Updates the FPS value.
         * @param newFpsValue - The new FPS value.
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::UpdateFps(int newFpsValue, bool& success)
        {
            if (newFpsValue < 0)
            {
                LOGERR("Invalid FPS value: %d", newFpsValue);
                success = false;
                return Core::ERROR_GENERAL;
            }

            {
                std::lock_guard<std::mutex> guard(m_callMutex);

                m_minFpsValue = std::min(m_minFpsValue, newFpsValue);
                m_maxFpsValue = std::max(m_maxFpsValue, newFpsValue);

                m_totalFpsValues += newFpsValue;
                m_numberOfFpsUpdates++;
                m_lastFpsValue = newFpsValue;
            }

            DBGINFO("FPS updated: newFpsValue=%d, minFps=%d, maxFps=%d, totalFps=%d, updates=%d",
                    newFpsValue, m_minFpsValue, m_maxFpsValue, m_totalFpsValues, m_numberOfFpsUpdates);

            success = true;
            return Core::ERROR_NONE;
        }

        /**
         * @brief This function is used to get the amount of collection interval per milliseconds.
         * @param frequency - The amount of time in milliseconds.
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::GetCollectionFrequency(int& frequency, bool& success)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            frequency = m_fpsCollectionFrequencyInMs;
            success = true;
            return Core::ERROR_NONE;
        }

		/************************************** Implementation specific *********************************************/

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

            dispatchOnFpsEvent(averageFps, minFps, maxFps);

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
    } // namespace Plugin
} // namespace WPEFramework
