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
#include <regex>
#include <sstream>
#include <limits>
#include <cstring>

#include "FrameRateImplementation.h"
#include "host.hpp"
#include "exception.hpp"

#include "UtilsJsonRpc.h"
#include "UtilsIarm.h"
#include "UtilsProcess.h"

//Defines
#define DEFAULT_FPS_COLLECTION_TIME_IN_MILLISECONDS 10000
#define MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS 100
#define DEFAULT_MIN_FPS_VALUE 60
#define DEFAULT_MAX_FPS_VALUE -1

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
            device::Host::getInstance().Register(this, "WPE::FrameRate");
            // Connect the timer callback handle for triggering FrameRate notifications.
            m_reportFpsTimer.connect(std::bind(&FrameRateImplementation::onReportFpsTimer, this));
        }

        FrameRateImplementation::~FrameRateImplementation()
        {
            device::Host::getInstance().UnRegister(this);
            //Stop the timer if running
            if (m_reportFpsTimer.isActive())
            {
                m_reportFpsTimer.stop();
            }
        }

        /******************************************* Notifications ****************************************/

        /**
         * @brief This function is used to dispatch 'onFpsEvent' to all registered notification sinks.
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
            DBGINFO("displayFrameRate: '%s'", displayFrameRate.c_str());
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
            DBGINFO("displayFrameRate: '%s'", displayFrameRate.c_str());
        }

        /**
         * @brief This function is used to dispatch the display frame rate change event, triggered by worker pool.
         * @param event - The event type (prechange or postchange).
         * @param params - The parameters associated with the event.
         * @return void
         */
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

        Core::hresult FrameRateImplementation::Register(Exchange::IFrameRate::INotification *notification)
        {
            Core::hresult status = Core::ERROR_NONE;
            ASSERT(nullptr != notification);
            _adminLock.Lock();

            // Check if the notification is already registered
            if (std::find(_framerateNotification.begin(), _framerateNotification.end(), notification) != _framerateNotification.end())
            {
                LOGERR("Same notification is registered already");
                status = Core::ERROR_ALREADY_CONNECTED;
            }

            _framerateNotification.push_back(notification);
            notification->AddRef();

            _adminLock.Unlock();
            return status;
        }

        Core::hresult FrameRateImplementation::Unregister(Exchange::IFrameRate::INotification *notification)
        {
            Core::hresult status = Core::ERROR_GENERAL;
            ASSERT(nullptr != notification);
            _adminLock.Lock();

            // Just unregister one notification once
            auto itr = std::find(_framerateNotification.begin(), _framerateNotification.end(), notification);
            if (itr != _framerateNotification.end())
            {
                (*itr)->Release();
                _framerateNotification.erase(itr);
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("Notification handle %p not found in registered _framerateNotification list.\n", notification);
            }

            _adminLock.Unlock();
            return status;
        }

        /***************************************** Methods **********************************************/

        /**
         * @brief Returns the current display frame rate values.
         * @param framerate - The current display framerate setting (width x height x framerate)
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::GetDisplayFrameRate(string& framerate, bool& success)
        {
            DBGINFO();
            success = false;
            framerate.clear(); // Initialize output parameter

            std::lock_guard<std::mutex> guard(m_callMutex);

            try
            {
                device::List<device::VideoDevice> videoDevices = device::Host::getInstance().getVideoDevices();
                if (videoDevices.size() == 0)
                {
                    LOGERR("No video devices available.");
                    return Core::ERROR_NOT_SUPPORTED;
                }

                char sFramerate[64] = {0};
                device::VideoDevice& device = videoDevices.at(0);
                int result = device.getCurrentDisframerate(sFramerate);
                if (result == 0)
                {
                    // Ensure null termination for safety
                    sFramerate[sizeof(sFramerate) - 1] = '\0';

                    if (sFramerate[0] == '\0')
                    {
                        LOGERR("getCurrentDisframerate returned empty response.");
                        return Core::ERROR_GENERAL;
                    }

                    // sFramerate is expected to be in "WIDTHxHEIGHTpxFPS" format as per HAL Spec, eg: 3840x2160px48
                    std::string rawResponse(sFramerate);
                    LOGINFO("Raw getCurrentDisframerate response: '%s'", rawResponse.c_str());

                    // Validate input length to prevent regex DoS
                    if (rawResponse.length() > (sizeof(sFramerate) - 1))
                    {
                        LOGERR("Response string too long: %zu characters (max allowed: %zu)",
                               rawResponse.length(), (sizeof(sFramerate) - 1));
                        return Core::ERROR_PARSE_FAILURE;
                    }

                    // Use regex to parse and validate the format: WIDTHxHEIGHTpxFPS
                    // Strict pattern: only digits and optional decimal point for FPS, no trailing characters
                    std::regex formatRegex(R"(^(\d{1,5})x(\d{1,5})px(\d{1,5}(?:\.\d+)?)$)");
                    std::smatch matches;

                    if (std::regex_match(rawResponse, matches, formatRegex) && matches.size() >= 4)
                    {
                        try
                        {
                            // Extract captured groups for validation and conversion
                            const std::string& widthStr = matches[1].str();
                            const std::string& heightStr = matches[2].str();
                            const std::string& fpsStr = matches[3].str();

                            // Safe conversion with overflow checking
                            long widthLong = std::stol(widthStr);
                            long heightLong = std::stol(heightStr);
                            double fpsDouble = std::stod(fpsStr);
                            LOG_DBG("Parsed values: width=%ld, height=%ld, fps=%.f", widthLong, heightLong, fpsDouble);

                            // Validate ranges to prevent overflow and ensure reasonable values
                            constexpr long MAX_DIMENSION = std::numeric_limits<short>::max();
                            constexpr double MAX_FPS = 1000.0;
                            constexpr double MIN_FPS = 0.1;

                            if (widthLong <= 0 || widthLong > MAX_DIMENSION ||
                                heightLong <= 0 || heightLong > MAX_DIMENSION ||
                                fpsDouble < MIN_FPS || fpsDouble > MAX_FPS)
                            {
                                LOGERR("Invalid parsed values: width=%ld, height=%ld, fps=%.f", widthLong, heightLong, fpsDouble);
                                return Core::ERROR_PARSE_FAILURE;
                            }

                            // Safe cast after validation
                            int width = static_cast<int>(widthLong);
                            int height = static_cast<int>(heightLong);
                            float fps = static_cast<float>(fpsDouble);

                            // Format as clean "WIDTHxHEIGHTxFPS" without 'px' and trailing chars
                            std::ostringstream oss;
                            oss << width << "x" << height << "x" << std::fixed << std::setprecision(0) << fps;
                            framerate = oss.str();

                            LOGINFO("Parsed framerate successfully: '%s'\n", framerate.c_str());
                            success = true;
                            return Core::ERROR_NONE;
                        }
                        catch (const std::out_of_range& e)
                        {
                            LOGERR("Numeric overflow in response parsing: %s", e.what());
                            return Core::ERROR_PARSE_FAILURE;
                        }
                        catch (const std::invalid_argument& e)
                        {
                            LOGERR("Invalid numeric format in response: %s", e.what());
                            return Core::ERROR_PARSE_FAILURE;
                        }
                        catch (const std::exception& e)
                        {
                            LOGERR("Failed to parse numeric values from response: %s", e.what());
                            return Core::ERROR_PARSE_FAILURE;
                        }
                    }
                    else
                    {
                        LOGERR("Response format does not match expected 'WIDTHxHEIGHTpxFPS' pattern: '%s'",
                               rawResponse.c_str());
                        return Core::ERROR_PARSE_FAILURE;
                    }
                }
                else
                {
                    LOGERR("getCurrentDisframerate failed with error code: %d", result);
                    return Core::ERROR_GENERAL;
                }
            }
            catch (const device::Exception& err)
            {
                LOG_DEVICE_EXCEPTION0();
                return Core::ERROR_GENERAL;
            }
            catch (const std::exception& e)
            {
                LOGERR("Unexpected exception in GetDisplayFrameRate: %s", e.what());
                return Core::ERROR_GENERAL;
            }
            catch (...)
            {
                LOGERR("Unknown exception in GetDisplayFrameRate");
                return Core::ERROR_GENERAL;
            }
            catch(const std::exception& err)
            {
                LOGERR("exception: %s", err.what());
                success = false;
            }
            catch(...)
            {
                LOGWARN("Unknown exception occurred");
                success = false;
            }

            // Should not reach here, but for completeness
            return Core::ERROR_GENERAL;
        }

        /**
         * @brief Returns the current auto framerate mode.
         * @param autoFRMMode - The current auto framerate mode.
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::GetFrmMode(int &autoFRMMode, bool& success)
        {
            DBGINFO();
            std::lock_guard<std::mutex> guard(m_callMutex);

            success = false;
            try
            {
                device::List<device::VideoDevice> videoDevices = device::Host::getInstance().getVideoDevices();
                if (videoDevices.size() == 0)
                {
                    LOGERR("No video devices available.");
                    return Core::ERROR_NOT_SUPPORTED;
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
                LOG_DEVICE_EXCEPTION0();
            }
            catch(const std::exception& err)
            {
                LOGERR("exception: %s", err.what());
                success = false;
            }
            catch(...)
            {
                LOGWARN("Unknown exception occurred");
                success = false;
            }
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
            DBGINFO();
            success = false;
            if (frequency < MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS)
            {
                LOGERR("Invalid frequency, minimum is %d ms.", MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS);
                return Core::ERROR_INVALID_PARAMETER;
            }

            std::lock_guard<std::mutex> guard(m_callMutex);
            try
            {
                m_fpsCollectionFrequencyInMs = frequency;
                DBGINFO("FrameRate collection frequency set to %d milliseconds.", frequency);
                success = true;
                return Core::ERROR_NONE;
            }
            catch (const device::Exception& err)
            {
                LOG_DEVICE_EXCEPTION0();
            }
            return Core::ERROR_GENERAL;
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
            // Eg: 1920px1080px60
            // check if we got two 'x' in the string at least.
            success = false;
            if (std::count(framerate.begin(), framerate.end(), 'x') != 2 ||
                    !isdigit(framerate.front()) || !isdigit(framerate.back()))
            {
                LOGERR("Invalid frame rate format: '%s'", framerate.c_str());
                return Core::ERROR_INVALID_PARAMETER;
            }
            string sFramerate = framerate;
            std::lock_guard<std::mutex> guard(m_callMutex);

            try
            {
                device::List<device::VideoDevice> videoDevices = device::Host::getInstance().getVideoDevices();
                if (videoDevices.size() == 0)
                {
                    LOGERR("No video devices available.");
                    return Core::ERROR_NOT_SUPPORTED;
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
                LOG_DEVICE_EXCEPTION0();
            }
            catch(const std::exception& err)
            {
                LOGERR("exception: %s", err.what());
                success = false;
            }
            catch(...)
            {
                LOGWARN("Unknown exception occurred");
                success = false;
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
            success = false;
            if (frmmode != 0 && frmmode != 1)
            {
                LOGERR("Invalid frame mode: %d", frmmode);
                return Core::ERROR_INVALID_PARAMETER;
            }

            std::lock_guard<std::mutex> guard(m_callMutex);

            try
            {
                device::List<device::VideoDevice> videoDevices = device::Host::getInstance().getVideoDevices();
                if (videoDevices.size() == 0)
                {
                    LOGERR("No video devices available.");
                    return Core::ERROR_NOT_SUPPORTED;
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
                LOG_DEVICE_EXCEPTION0();
            }
            catch(const std::exception& err)
            {
                LOGERR("exception: %s", err.what());
                success = false;
            }
            catch(...)
            {
                LOGWARN("Unknown exception occurred");
                success = false;
            }
            return Core::ERROR_GENERAL;
        }

        /**
         * @brief Starts the FPS data collection.
         * @param success - Indicates whether the operation was successful.
         * @return Core::ERROR_NONE on success, Core::ERROR_GENERAL on failure.
         */
        Core::hresult FrameRateImplementation::StartFpsCollection(bool& success)
        {
            DBGINFO();
            std::lock_guard<std::mutex> guard(m_callMutex);

            if (m_fpsCollectionInProgress)
            {
                DBGINFO("FPS collection is already in progress.");
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
            DBGINFO();
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
                    // Protect against division by zero and integer overflow
                    if (m_numberOfFpsUpdates > 0 && m_totalFpsValues >= 0 &&
                        m_totalFpsValues <= (std::numeric_limits<int>::max() - m_numberOfFpsUpdates))
                    {
                        averageFps = (m_totalFpsValues / m_numberOfFpsUpdates);
                    }
                    else
                    {
                        LOGERR("Invalid FPS calculation values: total=%d, updates=%d",
                               m_totalFpsValues, m_numberOfFpsUpdates);
                        averageFps = -1;
                    }
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
            DBGINFO();
            if (newFpsValue < 0)
            {
                LOGERR("Invalid FPS value: %d", newFpsValue);
                success = false;
                return Core::ERROR_INVALID_PARAMETER;
            }

            // Add reasonable upper limit for FPS values
            constexpr int MAX_REASONABLE_FPS = 10000;
            if (newFpsValue > MAX_REASONABLE_FPS)
            {
                LOGERR("FPS value too high: %d (max allowed: %d)", newFpsValue, MAX_REASONABLE_FPS);
                success = false;
                return Core::ERROR_INVALID_PARAMETER;
            }

            std::lock_guard<std::mutex> guard(m_callMutex);

            // Check for integer overflow before adding
            if (m_totalFpsValues > (std::numeric_limits<int>::max() - newFpsValue))
            {
                LOGWARN("Integer overflow would occur in FPS calculation. Resetting counters.\n");
                // Reset counters to prevent overflow
                m_totalFpsValues = newFpsValue;
                m_numberOfFpsUpdates = 1;
                m_minFpsValue = newFpsValue;
                m_maxFpsValue = newFpsValue;
            }
            else
            {
                m_maxFpsValue = std::max(m_maxFpsValue, newFpsValue);
                m_minFpsValue = std::min(m_minFpsValue, newFpsValue);
                m_totalFpsValues += newFpsValue;
                m_numberOfFpsUpdates++;
            }

            m_lastFpsValue = newFpsValue;
            DBGINFO("m_maxFpsValue = %d, m_minFpsValue = %d, m_totalFpsValues = %d, m_numberOfFpsUpdates = %d",
                    m_maxFpsValue, m_minFpsValue, m_totalFpsValues, m_numberOfFpsUpdates);

            success = true;
            return Core::ERROR_NONE;
        }

        /************************************** Implementation specific ****************************************/

        /**
         * @brief This function is used to handle the timer event for reporting FPS.
         * @return void
         */
        void FrameRateImplementation::onReportFpsTimer()
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            int averageFps = (m_numberOfFpsUpdates > 0) ? (m_totalFpsValues / m_numberOfFpsUpdates) : -1;
            int minFps = (m_numberOfFpsUpdates > 0) ? m_minFpsValue : DEFAULT_MIN_FPS_VALUE;
            int maxFps = (m_numberOfFpsUpdates > 0) ? m_maxFpsValue : DEFAULT_MAX_FPS_VALUE;

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

        void FrameRateImplementation::OnDisplayFrameratePreChange(const std::string& frameRate)
        {
            LOGINFO("Received OnDisplayFrameratePreChange callback");
            Core::IWorkerPool::Instance().Submit(FrameRateImplementation::Job::Create(FrameRateImplementation::_instance,
                                    FrameRateImplementation::DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE,
                                    frameRate));
        }

        void FrameRateImplementation::OnDisplayFrameratePostChange(const std::string& frameRate)
        {
            LOGINFO("Received OnDisplayFrameratePostChange callback");
            Core::IWorkerPool::Instance().Submit(FrameRateImplementation::Job::Create(FrameRateImplementation::_instance,
                                    FrameRateImplementation::DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE,
                                    frameRate));
        }
    } // namespace Plugin
} // namespace WPEFramework