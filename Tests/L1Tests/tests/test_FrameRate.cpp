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

    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{"
                                          "\"jsonrpc\":\"2.0\","
                                          "\"method\":\"client.events.onDisplayFrameRateChanging.onDisplayFrameRateChanging\","
                                          "\"params\":{\"displayFrameRate\":\"3840x2160px48\"}"
                                          "}"))); 
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    strcpy(eventData.data.DisplayFrameRateChange.framerate,"3840x2160px48");

    ASSERT_TRUE(_iarmDSFramerateEventHandler != nullptr);

    EVENT_SUBSCRIBE(0, _T("onDisplayFrameRateChanging"), _T("org.rdk.FrameRate"), message);

    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData , sizeof(eventData));

    EVENT_UNSUBSCRIBE(0, _T("onDisplayFrameRateChanging"), _T("org.rdk.FrameRate"), message);
}

TEST_F(FrameRateTest, onDisplayFrameRateChanged)
{
    ASSERT_TRUE(_iarmDSFramerateEventHandler != nullptr);
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));
                EXPECT_EQ(text, string(_T("{"
                                          "\"jsonrpc\":\"2.0\","
                                          "\"method\":\"client.events.onDisplayFrameRateChanged.onDisplayFrameRateChanged\","
                                          "\"params\":{\"displayFrameRate\":\"3840x2160px48\"}"
                                          "}")));
                return Core::ERROR_NONE;
            }));
    IARM_Bus_DSMgr_EventData_t eventData;
    strcpy(eventData.data.DisplayFrameRateChange.framerate,"3840x2160px48");

    ASSERT_TRUE(_iarmDSFramerateEventHandler != nullptr);

    EVENT_SUBSCRIBE(0, _T("onDisplayFrameRateChanged"), _T("client.events.onDisplayFrameRateChanged"), message);
    _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, &eventData , sizeof(eventData));

    EVENT_UNSUBSCRIBE(0, _T("onDisplayFrameRateChanged"), _T("client.events.onDisplayFrameRateChanged"), message);
}
