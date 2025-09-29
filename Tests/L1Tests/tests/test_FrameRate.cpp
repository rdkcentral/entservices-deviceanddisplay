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

/**
 * @brief Test the Information method to validate plugin description
 */
TEST_F(FrameRateTest, Information)
{
    string expectedInfo = "Plugin which exposes FrameRate related methods and notifications.";
    EXPECT_EQ(expectedInfo, plugin->Information());
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

// ======================== NEGATIVE TEST CASES ========================

/**
 * @brief Test setCollectionFrequency with invalid frequency (below minimum)
 */
TEST_F(FrameRateTest, setCollectionFrequency_InvalidFrequencyBelowMinimum)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":50}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setCollectionFrequency with invalid frequency (negative value)
 */
TEST_F(FrameRateTest, setCollectionFrequency_InvalidFrequencyNegative)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":-100}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setCollectionFrequency with missing frequency parameter
 */
TEST_F(FrameRateTest, setCollectionFrequency_MissingFrequencyParameter)
{
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setCollectionFrequency with invalid JSON
 */
TEST_F(FrameRateTest, setCollectionFrequency_InvalidJSON)
{
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test updateFps with negative value
 */
TEST_F(FrameRateTest, updateFps_NegativeValue)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":-10}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test updateFps with missing newFpsValue parameter
 */
TEST_F(FrameRateTest, updateFps_MissingParameter)
{
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("updateFps"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setDisplayFrameRate with invalid format (missing 'x' separators)
 */
TEST_F(FrameRateTest, setDisplayFrameRate_InvalidFormatMissingX)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840-2160-48\"}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setDisplayFrameRate with invalid format (only one 'x')
 */
TEST_F(FrameRateTest, setDisplayFrameRate_InvalidFormatOneX)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160-48\"}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setDisplayFrameRate with invalid format (too many 'x')
 */
TEST_F(FrameRateTest, setDisplayFrameRate_InvalidFormatTooManyX)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160x48x60\"}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setDisplayFrameRate with invalid format (starts with non-digit)
 */
TEST_F(FrameRateTest, setDisplayFrameRate_InvalidFormatStartsWithNonDigit)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"x3840x2160x48\"}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setDisplayFrameRate with invalid format (ends with non-digit)
 */
TEST_F(FrameRateTest, setDisplayFrameRate_InvalidFormatEndsWithNonDigit)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160x48x\"}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setDisplayFrameRate with empty string
 */
TEST_F(FrameRateTest, setDisplayFrameRate_EmptyString)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"\"}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setDisplayFrameRate with missing framerate parameter
 */
TEST_F(FrameRateTest, setDisplayFrameRate_MissingParameter)
{
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setDisplayFrameRate when no video devices are available
 */
TEST_F(FrameRateTest, setDisplayFrameRate_NoVideoDevicesAvailable)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160x48\"}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setDisplayFrameRate when device setDisplayframerate fails
 */
TEST_F(FrameRateTest, setDisplayFrameRate_DeviceSetDisplayFramerateFails)
{
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* param) {
                return 1; // Return non-zero to indicate failure
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160x48\"}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setDisplayFrameRate when device throws exception
 */
TEST_F(FrameRateTest, setDisplayFrameRate_DeviceThrowsException)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Invoke(
            [&]() -> device::List<device::VideoDevice> {
                throw device::Exception("Device exception occurred");
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160x48\"}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setFrmMode when no video devices are available
 */
TEST_F(FrameRateTest, setFrmMode_NoVideoDevicesAvailable)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":0}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setFrmMode when device setFRFMode fails
 */
TEST_F(FrameRateTest, setFrmMode_DeviceSetFRFModeFails)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int param) {
                return 1; // Return non-zero to indicate failure
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":0}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setFrmMode when device throws exception
 */
TEST_F(FrameRateTest, setFrmMode_DeviceThrowsException)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Invoke(
            [&]() -> device::List<device::VideoDevice> {
                throw device::Exception("Device exception occurred");
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":0}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test setFrmMode with missing frmmode parameter
 */
TEST_F(FrameRateTest, setFrmMode_MissingParameter)
{
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("setFrmMode"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test getFrmMode when no video devices are available
 */
TEST_F(FrameRateTest, getFrmMode_NoVideoDevicesAvailable)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test getFrmMode when device getFRFMode fails
 */
TEST_F(FrameRateTest, getFrmMode_DeviceGetFRFModeFails)
{
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int* param) {
                return 1; // Return non-zero to indicate failure
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test getFrmMode when device throws exception
 */
TEST_F(FrameRateTest, getFrmMode_DeviceThrowsException)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Invoke(
            [&]() -> device::List<device::VideoDevice> {
                throw device::Exception("Device exception occurred");
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test getDisplayFrameRate when no video devices are available
 */
TEST_F(FrameRateTest, getDisplayFrameRate_NoVideoDevicesAvailable)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test getDisplayFrameRate when device getCurrentDisframerate fails
 */
TEST_F(FrameRateTest, getDisplayFrameRate_DeviceGetCurrentDisframerateFails)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](char* param) {
                return 1; // Return non-zero to indicate failure
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test getDisplayFrameRate when device throws exception
 */
TEST_F(FrameRateTest, getDisplayFrameRate_DeviceThrowsException)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Invoke(
            [&]() -> device::List<device::VideoDevice> {
                throw device::Exception("Device exception occurred");
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(response, string(""));
}

/**
 * @brief Test invalid JSON input for all methods
 */
TEST_F(FrameRateTest, InvalidJSON_AllMethods)
{
    // Test with malformed JSON
    string invalidJson = "{\"invalid\":}";
    
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("setCollectionFrequency"), invalidJson, response));
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("startFpsCollection"), invalidJson, response));
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("stopFpsCollection"), invalidJson, response));
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("updateFps"), invalidJson, response));
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("setFrmMode"), invalidJson, response));
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("getFrmMode"), invalidJson, response));
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("setDisplayFrameRate"), invalidJson, response));
    EXPECT_EQ(Core::ERROR_INVALID_INPUT, handler.Invoke(connection, _T("getDisplayFrameRate"), invalidJson, response));
}

/**
 * @brief Test with non-existent method
 */
TEST_F(FrameRateTest, NonExistentMethod)
{
    EXPECT_EQ(Core::ERROR_INVALID_DESIGNATOR, handler.Invoke(connection, _T("nonExistentMethod"), _T("{}"), response));
}
