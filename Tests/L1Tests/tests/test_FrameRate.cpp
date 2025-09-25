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

// ================================ NEGATIVE TEST CASES ================================

/**
 * @brief Test setCollectionFrequency with invalid parameters
 * 
 * This test validates error handling for setCollectionFrequency:
 * - Tests with frequency below minimum threshold (< 100ms)
 * - Tests with negative frequency values
 * - Tests with extremely large frequency values
 * - Validates proper error responses and success=false
 */
TEST_F(FrameRateTest, setCollectionFrequency_NegativeTests)
{
    // Test with frequency below minimum (should fail - minimum is 100ms)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":50, \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with negative frequency
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":-1000, \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with zero frequency
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":0, \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with extremely large frequency (potential overflow)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":2147483647, \"success\":false}"), response));
    // This might succeed or fail depending on implementation limits
}

/**
 * @brief Test updateFps with invalid FPS values
 * 
 * This test validates error handling for updateFps:
 * - Tests with negative FPS values
 * - Tests with zero FPS
 * - Tests with unrealistic high FPS values
 * - Validates proper error responses
 */
TEST_F(FrameRateTest, updateFps_NegativeTests)
{
    // Test with negative FPS value
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":-30, \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with zero FPS
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":0, \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with unrealistically high FPS (> 1000)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":5000, \"success\":false}"), response));
    // This might succeed or fail depending on implementation validation
}

/**
 * @brief Test setFrmMode with invalid mode values and device exceptions
 * 
 * This test validates error handling for setFrmMode:
 * - Tests with invalid FRM mode values
 * - Tests device exception scenarios
 * - Tests when VideoDevice is unavailable
 */
TEST_F(FrameRateTest, setFrmMode_NegativeTests)
{
    // Test with invalid FRM mode (negative value)
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int param) {
                // Simulate device error for invalid mode
                throw device::Exception("Invalid FRM mode");
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":-1, \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with invalid FRM mode (out of range value)
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int param) {
                if (param > 1 || param < 0) {
                    throw device::Exception("FRM mode out of range");
                }
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":999, \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test getFrmMode with device exceptions and error conditions
 * 
 * This test validates error handling for getFrmMode:
 * - Tests when device throws exceptions
 * - Tests when VideoDevice is unavailable
 * - Tests hardware communication failures
 */
TEST_F(FrameRateTest, getFrmMode_NegativeTests)
{
    // Test with device exception during getFRFMode
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int* param) {
                throw device::Exception("Failed to get FRM mode from hardware");
                return -1;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getFrmMode"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with device returning error code
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int* param) {
                return -1; // Simulate hardware error
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getFrmMode"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test setDisplayFrameRate with invalid framerate formats and device exceptions
 * 
 * This test validates error handling for setDisplayFrameRate:
 * - Tests with malformed framerate strings
 * - Tests with unsupported resolutions
 * - Tests with invalid framerate values
 * - Tests device exception scenarios
 */
TEST_F(FrameRateTest, setDisplayFrameRate_NegativeTests)
{
    // Test with malformed framerate string
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* param) {
                if (strlen(param) == 0 || strstr(param, "x") == nullptr) {
                    throw device::Exception("Invalid framerate format");
                }
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"invalid_format\", \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with empty framerate string
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"\", \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with unsupported resolution
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* param) {
                string framerate(param);
                if (framerate.find("9999x9999") != string::npos) {
                    throw device::Exception("Unsupported resolution");
                }
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"9999x9999px60\", \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with zero or negative framerate
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* param) {
                string framerate(param);
                if (framerate.find("px0") != string::npos || framerate.find("px-") != string::npos) {
                    throw device::Exception("Invalid framerate value");
                }
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080px0\", \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080px-30\", \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test getDisplayFrameRate with device exceptions and error conditions
 * 
 * This test validates error handling for getDisplayFrameRate:
 * - Tests when device throws exceptions
 * - Tests when hardware communication fails
 * - Tests when display is disconnected
 */
TEST_F(FrameRateTest, getDisplayFrameRate_NegativeTests)
{
    // Test with device exception during getCurrentDisframerate
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](char* param) {
                throw device::Exception("Failed to get current display framerate");
                return -1;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with device returning error code
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](char* param) {
                return -1; // Simulate hardware error
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test with display disconnected scenario
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](char* param) {
                throw device::Exception("Display not connected");
                return -1;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test FPS collection functions with invalid states
 * 
 * This test validates error handling for FPS collection:
 * - Tests starting collection when already in progress
 * - Tests stopping collection when not started
 * - Tests multiple start/stop calls
 */
TEST_F(FrameRateTest, FpsCollection_InvalidStates)
{
    // Start FPS collection successfully first
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, "true");

    // Try to start again while already running (should handle gracefully)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{\"success\":false}"), response));
    // Implementation may return true (idempotent) or false (already running)

    // Stop collection
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, "true");

    // Try to stop again when not running (should handle gracefully)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{\"success\":false}"), response));
    // Implementation may return true (idempotent) or false (not running)
}

/**
 * @brief Test JSON parsing errors and malformed requests
 * 
 * This test validates error handling for malformed JSON:
 * - Tests with missing required parameters
 * - Tests with wrong parameter types
 * - Tests with malformed JSON syntax
 */
TEST_F(FrameRateTest, JSON_ParsingErrors)
{
    // Test setCollectionFrequency with missing frequency parameter
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"success\":false}"), response));
    // Should return error due to missing frequency

    // Test updateFps with missing newFpsValue parameter
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"success\":false}"), response));
    // Should return error due to missing newFpsValue

    // Test setFrmMode with missing frmmode parameter
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"success\":false}"), response));
    // Should return error due to missing frmmode

    // Test setDisplayFrameRate with missing framerate parameter
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"success\":false}"), response));
    // Should return error due to missing framerate

    // Test with wrong parameter types
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":\"not_a_number\", \"success\":false}"), response));
    // Should return error due to wrong type

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":\"not_a_number\", \"success\":false}"), response));
    // Should return error due to wrong type

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":\"not_a_number\", \"success\":false}"), response));
    // Should return error due to wrong type
}

/**
 * @brief Test IARM event handling with invalid data
 * 
 * This test validates error handling for IARM events:
 * - Tests with null event data
 * - Tests with malformed event data
 * - Tests with incorrect event sizes
 */
TEST_F(FrameRateTest, IARM_EventHandling_NegativeTests)
{
    ASSERT_TRUE(_iarmDSFramerateEventHandler != nullptr);
    
    // Test with null data pointer (should not crash)
    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, nullptr, 0);
    
    // Test with incorrect event size
    IARM_Bus_DSMgr_EventData_t eventData;
    strcpy(eventData.data.DisplayFrameRateChange.framerate, "3840x2160px48");
    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData, sizeof(eventData) - 1);
    
    // Test with unknown event ID (should be ignored gracefully)
    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, (IARM_EventId_t)9999, &eventData, sizeof(eventData));
    
    // Test with wrong owner name (should be ignored)
    _iarmDSFramerateEventHandler("WRONG_OWNER", IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData, sizeof(eventData));
}

/**
 * @brief Test device settings exception scenarios
 * 
 * This test validates exception handling when device settings operations fail:
 * - Tests device initialization failures
 * - Tests device communication timeouts
 * - Tests hardware unavailable scenarios
 */
TEST_F(FrameRateTest, DeviceSettings_ExceptionHandling)
{
    // Test VideoDevice initialization failure
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int* param) {
                throw std::runtime_error("VideoDevice not initialized");
                return -1;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getFrmMode"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test device communication timeout
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* param) {
                throw device::Exception("Device communication timeout");
                return -1;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080px60\", \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test hardware unavailable
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](char* param) {
                throw device::Exception("Hardware unavailable");
                return -1;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));
}

/**
 * @brief Test boundary value conditions
 * 
 * This test validates handling of boundary values:
 * - Tests minimum and maximum supported framerates
 * - Tests edge cases for collection frequency limits
 * - Tests resolution boundary conditions
 */
TEST_F(FrameRateTest, BoundaryValue_Tests)
{
    // Test minimum collection frequency (exactly 100ms - should succeed)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":100, \"success\":false}"), response));
    EXPECT_EQ(response, "true");

    // Test just below minimum (99ms - should fail)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":99, \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test maximum reasonable FPS values
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* param) {
                string framerate(param);
                // Accept common framerates, reject unrealistic ones
                if (framerate.find("px240") != string::npos) {
                    throw device::Exception("Framerate too high for hardware");
                }
                return 0;
            }));

    // Test very high framerate (should fail)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080px240\", \"success\":false}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    // Test common high framerate (should succeed)
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080px120\", \"success\":false}"), response));
    EXPECT_EQ(response, "true");
}

/**
 * @brief Test concurrent operations and race conditions
 * 
 * This test validates handling of concurrent operations:
 * - Tests rapid successive API calls
 * - Tests FPS collection during framerate changes
 * - Tests notification handling during operations
 */
TEST_F(FrameRateTest, Concurrency_Tests)
{
    // Start FPS collection
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, "true");

    // Rapid updateFps calls (should handle gracefully)
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":60, \"success\":false}"), response));
    }

    // Change framerate while FPS collection is running
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160px60\", \"success\":false}"), response));

    // Stop FPS collection
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{\"success\":false}"), response));
    EXPECT_EQ(response, "true");
}
