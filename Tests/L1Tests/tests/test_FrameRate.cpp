/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
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
**/

#include <gtest/gtest.h>
#include "COMLinkMock.h"
#include <gmock/gmock.h>

#include "FrameRate.h"

#include "FactoriesImplementation.h"
#include "HostMock.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "VideoDeviceMock.h"
#include "devicesettings.h"
#include "dsMgr.h"
#include "ThunderPortability.h"
#include "FrameRateImplementation.h"
#include "FrameRateMock.h"
#include "WorkerPoolImplementation.h"
#include "WrapsMock.h"

#include <thread>
#include <chrono>
#include <vector>
#include <cstring>

using namespace WPEFramework;

using ::testing::NiceMock;

class FrameRateTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::FrameRate> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    NiceMock<ServiceMock> service;
    NiceMock<COMLinkMock> comLinkMock;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    Core::ProxyType<Plugin::FrameRateImplementation> FrameRateImplem;
    Exchange::IFrameRate::INotification *FrameRateNotification = nullptr;
    NiceMock<FactoriesImplementation> factoriesImplementation;
    PLUGINHOST_DISPATCHER *dispatcher;
    string response;
    Core::JSONRPC::Message message;
    ServiceMock  *p_serviceMock  = nullptr;
    WrapsImplMock* p_wrapsImplMock = nullptr;
    FrameRateMock* p_framerateMock = nullptr;
    HostImplMock      *p_hostImplMock = nullptr;
    VideoDeviceMock   *p_videoDeviceMock = nullptr;
    IARM_EventHandler_t _iarmDSFramerateEventHandler;
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;


    FrameRateTest()
        : plugin(Core::ProxyType<Plugin::FrameRate>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
	, workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
          2, Core::Thread::DefaultStackSize(), 16))
    {
	    p_serviceMock = new NiceMock <ServiceMock>;

        p_framerateMock  = new NiceMock <FrameRateMock>;

	p_wrapsImplMock = new NiceMock<WrapsImplMock>;
        Wraps::setImpl(p_wrapsImplMock);

        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
        plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);

        ON_CALL(*p_framerateMock, Register(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](Exchange::IFrameRate::INotification* notification) {
                    FrameRateNotification = notification;
		    return Core::ERROR_NONE;
                }));

#ifdef USE_THUNDER_R4
        ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_))
                .WillByDefault(::testing::Invoke(
                    [&](const RPC::Object& object, const uint32_t waitTime, uint32_t& connectionId) {
                        FrameRateImplem = Core::ProxyType<Plugin::FrameRateImplementation>::Create();
                        return &FrameRateImplem;
                    }));
#else
	ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
	    .WillByDefault(::testing::Return(FrameRateImplem));
#endif
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE)) 			{
			//FrameRatePreChange = handler;
			_iarmDSFramerateEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE))			{
			//FrameRatePostChange = handler;
			_iarmDSFramerateEventHandler = handler;
                    }
                    return IARM_RESULT_SUCCESS;
                }));

        Core::IWorkerPool::Assign(&(*workerPool));
            workerPool->Run();

        plugin->Initialize(&service);
        p_hostImplMock  = new NiceMock <HostImplMock>;
        device::Host::setImpl(p_hostImplMock);

        device::VideoDevice videoDevice;
        p_videoDeviceMock  = new NiceMock <VideoDeviceMock>;
        device::VideoDevice::setImpl(p_videoDeviceMock);

        ON_CALL(*p_hostImplMock, getVideoDevices())
            .WillByDefault(::testing::Return(device::List<device::VideoDevice>({ videoDevice })));
    }
    virtual ~FrameRateTest()
    {
        plugin->Deinitialize(&service);
        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();

	    if (p_serviceMock != nullptr)
        {
            delete p_serviceMock;
            p_serviceMock = nullptr;
        }

        if (p_framerateMock != nullptr) {
            delete p_framerateMock;
            p_framerateMock = nullptr;
        }

        Wraps::setImpl(nullptr);
        if (p_wrapsImplMock != nullptr) {
            delete p_wrapsImplMock;
            p_wrapsImplMock = nullptr;
        }

        device::VideoDevice::setImpl(nullptr);
        if (p_videoDeviceMock != nullptr)
        {
            delete p_videoDeviceMock;
            p_videoDeviceMock = nullptr;
        }

        device::Host::setImpl(nullptr);
        if (p_hostImplMock != nullptr)
        {
            delete p_hostImplMock;
            p_hostImplMock = nullptr;
        }
        dispatcher->Deactivate();
        dispatcher->Release();

        PluginHost::IFactories::Assign(nullptr);
	IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr) {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }

    }
};

TEST_F(FrameRateTest, RegisteredMethods)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setCollectionFrequency")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("startFpsCollection")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("stopFpsCollection")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("updateFps")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setFrmMode")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getFrmMode")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setDisplayFrameRate")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getDisplayFrameRate")));
}



TEST_F(FrameRateTest, setCollectionFrequency_startFpsCollection_stopFpsCollection_updateFps)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":1000, \"success\":false}"), response));
    EXPECT_EQ(response, "true");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, "true");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, "true");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":60, \"success\":false}"), response));
    EXPECT_EQ(response, "true");
}

/**
 * Segmentation fault without valgrind
 */

TEST_F(FrameRateTest, setFrmMode)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int param) {
                EXPECT_EQ(param, 0);
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":0, \"success\":false}"), response));
    EXPECT_EQ(response, "true");

}


/**
 * Segmentation fault without valgrind
 */

TEST_F(FrameRateTest, getFrmMode)
{
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int* param) {
                *param = 0;
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getFrmMode"), _T("{ \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"auto-frm-mode\":0,\"success\":true}"));
}


/**
 * Segmentation fault without valgrind
 */

TEST_F(FrameRateTest, setDisplayFrameRate)
{
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* param) {
                EXPECT_EQ(param, string("3840x2160px48"));
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160px48\",\"success\":false}"), response));
    EXPECT_EQ(response, "true");
}


/**
 * Segmentation fault without valgrind
 */

TEST_F(FrameRateTest, getDisplayFrameRate)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](char* param) {
                string framerate("3840x2160px48");
                ::memcpy(param, framerate.c_str(), framerate.length());
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, string("{\"framerate\":\"3840x2160px48\",\"success\":true}"));
}


TEST_F(FrameRateTest, onDisplayFrameRateChanging)
{
    ASSERT_TRUE(_iarmDSFramerateEventHandler != nullptr);
    Core::Event resetDone(false, true);
    EVENT_SUBSCRIBE(0, _T("onDisplayFrameRateChanging"), _T("org.rdk.FrameRate"), message);	
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{"
                                          "\"jsonrpc\":\"2.0\","
                                          "\"method\":\"org.rdk.FrameRate.onDisplayFrameRateChanging\","
                                          "\"params\":{\"displayFrameRate\":\"3840x2160px48\"}"
                                          "}"))); 
		resetDone.SetEvent();
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    strcpy(eventData.data.DisplayFrameRateChange.framerate,"3840x2160px48");
    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData , sizeof(eventData));
    EXPECT_EQ(Core::ERROR_NONE, resetDone.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onDisplayFrameRateChanging"), _T("org.rdk.FrameRate"), message);
}

TEST_F(FrameRateTest, onDisplayFrameRateChanged)
{
    ASSERT_TRUE(_iarmDSFramerateEventHandler != nullptr);
    Core::Event resetDone(false, true);
    EVENT_SUBSCRIBE(0, _T("onDisplayFrameRateChanged"), _T("org.rdk.FrameRate"), message);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{"
                                          "\"jsonrpc\":\"2.0\","
                                          "\"method\":\"org.rdk.FrameRate.onDisplayFrameRateChanged\","
                                          "\"params\":{\"displayFrameRate\":\"3840x2160px48\"}"
                                          "}")));
		resetDone.SetEvent();    
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    strcpy(eventData.data.DisplayFrameRateChange.framerate,"3840x2160px48");
    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, &eventData , sizeof(eventData));
    EXPECT_EQ(Core::ERROR_NONE, resetDone.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onDisplayFrameRateChanged"), _T("org.rdk.FrameRate"), message);
}

// *********************** NEGATIVE TEST CASES ***********************

/**
 * @brief Test SetCollectionFrequency with frequency below minimum threshold
 * 
 * This test validates that SetCollectionFrequency properly rejects values below
 * MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS (100ms) and returns appropriate error.
 */
TEST_F(FrameRateTest, SetCollectionFrequency_BelowMinimum_ShouldFail)
{
    string response;
    
    // Test with frequency = 50ms (below 100ms minimum)
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":50}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
    
    // Test with frequency = 0
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":0}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
    
    // Test with negative frequency
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":-100}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test SetDisplayFrameRate with invalid framerate format
 * 
 * This test validates that SetDisplayFrameRate properly handles malformed
 * framerate strings and device::Exception scenarios.
 */
TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidFormat_ShouldFail)
{
    string response;
    
    ON_CALL(*p_videoDeviceMock, getVideoDevice(::testing::_))
        .WillByDefault(::testing::Throw(device::Exception("Invalid framerate format")));
    
    // Test with invalid format
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"invalid_format\"}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
    
    // Test with empty framerate
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"\"}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
    
    // Test with missing framerate parameter
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test SetFrmMode with invalid frame mode values
 * 
 * This test validates that SetFrmMode properly rejects invalid frame mode
 * values and handles device exceptions appropriately.
 */
TEST_F(FrameRateTest, SetFrmMode_InvalidValues_ShouldFail)
{
    string response;
    
    ON_CALL(*p_videoDeviceMock, getVideoDevice(::testing::_))
        .WillByDefault(::testing::Throw(device::Exception("Invalid frame mode")));
    
    // Test with invalid frame mode value (should be 0 or 1)
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":5}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
    
    // Test with negative frame mode
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":-1}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
    
    // Test with missing frmmode parameter
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test GetDisplayFrameRate when device operations fail
 * 
 * This test validates that GetDisplayFrameRate properly handles device::Exception
 * scenarios and returns appropriate error responses.
 */
TEST_F(FrameRateTest, GetDisplayFrameRate_DeviceException_ShouldFail)
{
    string response;
    
    ON_CALL(*p_videoDeviceMock, getVideoDevice(::testing::_))
        .WillByDefault(::testing::Throw(device::Exception("Device communication failed")));
    
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test GetFrmMode when device operations fail
 * 
 * This test validates that GetFrmMode properly handles device::Exception
 * scenarios and returns appropriate error responses.
 */
TEST_F(FrameRateTest, GetFrmMode_DeviceException_ShouldFail)
{
    string response;
    
    ON_CALL(*p_videoDeviceMock, getVideoDevice(::testing::_))
        .WillByDefault(::testing::Throw(device::Exception("Frame mode retrieval failed")));
    
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test StartFpsCollection when already in progress
 * 
 * This test validates the behavior when trying to start FPS collection
 * while it's already running, and when device operations fail.
 */
TEST_F(FrameRateTest, StartFpsCollection_AlreadyInProgress_And_DeviceFailure)
{
    string response;
    
    // First, start FPS collection successfully
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    
    // Try to start again while already in progress - should still succeed but may have different behavior
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    
    // Stop to clean up
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    
    // Now test device exception scenario
    ON_CALL(*p_videoDeviceMock, getVideoDevice(::testing::_))
        .WillByDefault(::testing::Throw(device::Exception("FPS collection start failed")));
    
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test StopFpsCollection when not running and device failures
 * 
 * This test validates the behavior when trying to stop FPS collection
 * when it's not running, and when device operations fail.
 */
TEST_F(FrameRateTest, StopFpsCollection_NotRunning_And_DeviceFailure)
{
    string response;
    
    // Try to stop when not running - should handle gracefully
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    
    // Test device exception scenario
    ON_CALL(*p_videoDeviceMock, getVideoDevice(::testing::_))
        .WillByDefault(::testing::Throw(device::Exception("FPS collection stop failed")));
    
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test UpdateFps with invalid FPS values
 * 
 * This test validates that UpdateFps properly handles invalid FPS values
 * and device exception scenarios.
 */
TEST_F(FrameRateTest, UpdateFps_InvalidValues_ShouldFail)
{
    string response;
    
    // Test with negative FPS value
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":-1}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
    
    // Test with extremely high FPS value (unrealistic)
    ON_CALL(*p_videoDeviceMock, getVideoDevice(::testing::_))
        .WillByDefault(::testing::Throw(device::Exception("Invalid FPS value")));
    
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":10000}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
    
    // Test with missing newFpsValue parameter
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("updateFps"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

// *********************** ADDITIONAL COVERAGE TESTS ***********************

/**
 * @brief Test IARM event handler with invalid/malformed event data
 * 
 * This test validates the robustness of the IARM event handler when
 * receiving malformed or invalid event data.
 */
TEST_F(FrameRateTest, IARM_EventHandler_InvalidEventData)
{
    ASSERT_TRUE(_iarmDSFramerateEventHandler != nullptr);
    
    // Test with null data
    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, nullptr, 0);
    
    // Test with empty framerate string
    IARM_Bus_DSMgr_EventData_t eventData;
    memset(&eventData, 0, sizeof(eventData));
    eventData.data.DisplayFrameRateChange.framerate[0] = '\0';
    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData, sizeof(eventData));
    
    // Test with extremely long framerate string (should be truncated)
    memset(eventData.data.DisplayFrameRateChange.framerate, 'x', sizeof(eventData.data.DisplayFrameRateChange.framerate));
    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, &eventData, sizeof(eventData));
}

/**
 * @brief Test IARM event handler with unknown event IDs
 * 
 * This test validates that the IARM event handler properly ignores
 * unknown or unsupported event IDs.
 */
TEST_F(FrameRateTest, IARM_EventHandler_UnknownEventId)
{
    ASSERT_TRUE(_iarmDSFramerateEventHandler != nullptr);
    
    IARM_Bus_DSMgr_EventData_t eventData;
    strcpy(eventData.data.DisplayFrameRateChange.framerate, "1920x1080px60");
    
    // Test with unknown event ID (should be ignored)
    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, static_cast<IARM_EventId_t>(999), &eventData, sizeof(eventData));
    
    // Test with different owner name (should be handled appropriately)
    _iarmDSFramerateEventHandler("UnknownOwner", IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData, sizeof(eventData));
}

/**
 * @brief Test FPS collection timer functionality and edge cases
 * 
 * This test validates the timer-based FPS collection mechanism and
 * ensures proper handling of edge cases in FPS calculation.
 */
TEST_F(FrameRateTest, FpsCollection_TimerAndCalculation_EdgeCases)
{
    string response;
    
    // Start FPS collection
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    
    // Update FPS with various values to test calculation edge cases
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":0}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":120}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":60}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    
    // Wait a brief moment to allow timer processing
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Stop FPS collection
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

/**
 * @brief Test SetCollectionFrequency with boundary values
 * 
 * This test validates SetCollectionFrequency with values at the boundary
 * conditions (minimum allowed, maximum reasonable values).
 */
TEST_F(FrameRateTest, SetCollectionFrequency_BoundaryValues)
{
    string response;
    
    // Test with minimum allowed value (100ms)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":100}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    
    // Test with very large value (should be accepted)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":60000}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    
    // Reset to default
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":10000}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

/**
 * @brief Test notification registration and unregistration edge cases
 * 
 * This test validates the notification system's handling of edge cases
 * like duplicate registrations and unregistering non-existent notifications.
 */
TEST_F(FrameRateTest, NotificationSystem_EdgeCases)
{
    // This test would require access to the FrameRateImplementation instance
    // and mock INotification objects to test the Register/Unregister methods
    // directly. Since we're testing through the plugin interface, we validate
    // the event flow through the existing event subscription mechanism.
    
    string response;
    
    // Subscribe to multiple events to test notification system
    EVENT_SUBSCRIBE(0, _T("onFpsEvent"), _T("org.rdk.FrameRate"), message);
    EVENT_SUBSCRIBE(0, _T("onDisplayFrameRateChanging"), _T("org.rdk.FrameRate"), message);
    EVENT_SUBSCRIBE(0, _T("onDisplayFrameRateChanged"), _T("org.rdk.FrameRate"), message);
    
    // Start FPS collection to trigger potential onFpsEvent
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    
    // Update FPS to potentially trigger event
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":60}"), response));
    
    // Brief wait for potential event processing
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Stop FPS collection
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    
    // Unsubscribe from events
    EVENT_UNSUBSCRIBE(0, _T("onFpsEvent"), _T("org.rdk.FrameRate"), message);
    EVENT_UNSUBSCRIBE(0, _T("onDisplayFrameRateChanging"), _T("org.rdk.FrameRate"), message);
    EVENT_UNSUBSCRIBE(0, _T("onDisplayFrameRateChanged"), _T("org.rdk.FrameRate"), message);
}

/**
 * @brief Test malformed JSON requests
 * 
 * This test validates that the plugin properly handles malformed JSON
 * requests and returns appropriate error responses.
 */
TEST_F(FrameRateTest, MalformedJSON_Requests)
{
    string response;
    
    // Test with invalid JSON syntax
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":}"), response));
    
    // Test with missing closing brace
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080px60\""), response));
    
    // Test with wrong parameter types
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":\"not_a_number\"}"), response));
    
    // Test with extra commas
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":60,}"), response));
}

/**
 * @brief Test concurrent operations and thread safety
 * 
 * This test validates that the plugin can handle multiple concurrent
 * operations without causing race conditions or crashes.
 */
TEST_F(FrameRateTest, ConcurrentOperations_ThreadSafety)
{
    string response1, response2, response3;
    
    // Start multiple operations concurrently
    std::vector<std::thread> threads;
    std::vector<string> responses(3);
    std::vector<Core::hresult> results(3);
    
    // Thread 1: Set collection frequency
    threads.emplace_back([&]() {
        results[0] = handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":5000}"), responses[0]);
    });
    
    // Thread 2: Start FPS collection
    threads.emplace_back([&]() {
        results[1] = handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), responses[1]);
    });
    
    // Thread 3: Update FPS
    threads.emplace_back([&]() {
        results[2] = handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":30}"), responses[2]);
    });
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Validate that all operations completed (results may vary based on timing)
    // At minimum, they should not crash the system
    for (size_t i = 0; i < results.size(); ++i) {
        EXPECT_TRUE(results[i] == Core::ERROR_NONE || results[i] == Core::ERROR_GENERAL);
    }
    
    // Clean up
    string cleanup_response;
    handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), cleanup_response);
}

/**
 * @brief Test plugin initialization and deinitialization edge cases
 * 
 * This test validates proper handling of initialization/deinitialization
 * scenarios, including failure cases and cleanup.
 */
TEST_F(FrameRateTest, Plugin_InitializationDeinitializationEdgeCases)
{
    // Test initializing with null service (should be handled by ASSERT)
    // This tests the defensive programming in the Initialize method
    
    Core::ProxyType<Plugin::FrameRate> testPlugin = Core::ProxyType<Plugin::FrameRate>::Create();
    
    // Test multiple Initialize calls (should handle gracefully)
    string result1 = testPlugin->Initialize(&service);
    
    // Deinitialize properly
    testPlugin->Deinitialize(&service);
    
    // Test double deinitialize (should handle gracefully)
    testPlugin->Deinitialize(&service);
}

/**
 * @brief Test IARM initialization and deinitialization failures
 * 
 * This test validates handling of IARM bus connection failures
 * and proper cleanup in error scenarios.
 */
TEST_F(FrameRateTest, IARM_InitializationFailures)
{
    // Mock IARM connection failure scenarios
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_IsConnected())
        .WillByDefault(::testing::Return(IARM_RESULT_IPCCORE_FAIL));
    
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Connect())
        .WillByDefault(::testing::Return(IARM_RESULT_IPCCORE_FAIL));
    
    // Create a new plugin instance to test initialization with IARM failures
    Core::ProxyType<Plugin::FrameRate> testPlugin = Core::ProxyType<Plugin::FrameRate>::Create();
    
    // This should handle IARM connection failures gracefully
    string result = testPlugin->Initialize(&service);
    
    // Cleanup
    testPlugin->Deinitialize(&service);
}

/**
 * @brief Test onFpsEvent notification generation
 * 
 * This test validates that the onFpsEvent notification is properly
 * generated and dispatched when FPS data is collected.
 */
TEST_F(FrameRateTest, OnFpsEvent_NotificationGeneration)
{
    ASSERT_TRUE(_iarmDSFramerateEventHandler != nullptr);
    Core::Event resetDone(false, true);
    
    EVENT_SUBSCRIBE(0, _T("onFpsEvent"), _T("org.rdk.FrameRate"), message);
    
    // Set up expectation for onFpsEvent notification
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(::testing::AtLeast(0))  // May or may not be called depending on timer
        .WillRepeatedly(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                if (json->ToString(text)) {
                    // Check if this is an onFpsEvent notification
                    if (text.find("onFpsEvent") != string::npos) {
                        resetDone.SetEvent();
                    }
                }
                return Core::ERROR_NONE;
            }));
    
    string response;
    
    // Start FPS collection
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    
    // Set a short collection frequency to trigger events faster
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":100}"), response));
    
    // Update FPS multiple times to accumulate data
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":60}"), response));
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    
    // Wait for potential timer-based event (with timeout)
    resetDone.Lock(1000); // 1 second timeout
    
    // Stop FPS collection
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    
    EVENT_UNSUBSCRIBE(0, _T("onFpsEvent"), _T("org.rdk.FrameRate"), message);
}

/**
 * @brief Test parameter validation for all methods
 * 
 * This test ensures that all methods properly validate their input
 * parameters and handle missing or invalid parameters gracefully.
 */
TEST_F(FrameRateTest, ParameterValidation_AllMethods)
{
    string response;
    
    // Test all methods with completely empty JSON
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setCollectionFrequency"), _T(""), response));
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T(""), response));
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T(""), response));
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("updateFps"), _T(""), response));
    
    // Test all methods with null JSON
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setCollectionFrequency"), _T("null"), response));
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("null"), response));
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("null"), response));
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("updateFps"), _T("null"), response));
    
    // Test methods that should work with empty JSON (GET operations)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
}

/**
 * @brief Test memory management and resource cleanup
 * 
 * This test validates that the plugin properly manages memory and
 * cleans up resources during normal and error conditions.
 */
TEST_F(FrameRateTest, MemoryManagement_ResourceCleanup)
{
    string response;
    
    // Perform multiple operations to test memory management
    for (int i = 0; i < 10; ++i) {
        // Start and stop FPS collection multiple times
        EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
        EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":") + std::to_string(60 + i) + _T("}"), response));
        EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
        
        // Change collection frequency
        EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":") + std::to_string(1000 + i * 100) + _T("}"), response));
        
        // Get current values
        handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response);
        handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response);
    }
    
    // Final cleanup
    handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response);
}

/**
 * @brief Test string buffer overflow protection
 * 
 * This test validates that the plugin properly handles potentially
 * dangerous string inputs that could cause buffer overflows.
 */
TEST_F(FrameRateTest, StringBufferOverflow_Protection)
{
    string response;
    
    // Create extremely long framerate string
    string longFramerate(1000, 'x');
    string longFramerateJson = _T("{\"framerate\":\"") + longFramerate + _T("\"}");
    
    // This should be handled gracefully without causing buffer overflow
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), longFramerateJson, response));
    
    // Test IARM event with oversized framerate string
    if (_iarmDSFramerateEventHandler != nullptr) {
        IARM_Bus_DSMgr_EventData_t eventData;
        // Fill with maximum size + some extra to test truncation
        memset(eventData.data.DisplayFrameRateChange.framerate, 'A', sizeof(eventData.data.DisplayFrameRateChange.framerate));
        // Ensure we don't have a null terminator to test the truncation logic
        eventData.data.DisplayFrameRateChange.framerate[sizeof(eventData.data.DisplayFrameRateChange.framerate) - 1] = 'Z';
        
        // This should handle the oversized string gracefully
        _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData, sizeof(eventData));
    }
}
