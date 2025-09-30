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

// ...existing code...

TEST_F(FrameRateTest, GetDisplayFrameRate_Success)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke([](char* framerate) {
            strcpy(framerate, "1920x1080x60");
            return 0;
        }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_TRUE(response.find("\"success\":true") != string::npos);
    EXPECT_TRUE(response.find("\"framerate\":\"1920x1080x60\"") != string::npos);
}

TEST_F(FrameRateTest, GetDisplayFrameRate_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
}

TEST_F(FrameRateTest, GetDisplayFrameRate_DeviceFailure)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
}

TEST_F(FrameRateTest, GetFrmMode_Success)
{
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke([](int* frfmode) {
            *frfmode = 1;
            return 0;
        }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_TRUE(response.find("\"success\":true") != string::npos);
    EXPECT_TRUE(response.find("\"auto-frm-mode\":1") != string::npos);
}

TEST_F(FrameRateTest, GetFrmMode_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
}

TEST_F(FrameRateTest, GetFrmMode_DeviceFailure)
{
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
}

TEST_F(FrameRateTest, SetCollectionFrequency_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":5000}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetCollectionFrequency_MinimumValue)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":100}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetCollectionFrequency_InvalidParameter)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":50}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_Success)
{
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60\"}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidFormat_NoTwoX)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920-1080-60\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidFormat_ThreeX)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60x30\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidFormat_NonDigitStart)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"x1920x1080x60\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidFormat_NonDigitEnd)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60x\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_DeviceFailure)
{
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60\"}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_Success_Mode0)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":0}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetFrmMode_Success_Mode1)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":1}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetFrmMode_InvalidParameter_Negative)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":-1}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_InvalidParameter_GreaterThan1)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":2}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":1}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_DeviceFailure)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":1}"), response));
}

TEST_F(FrameRateTest, StartFpsCollection_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, StopFpsCollection_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, UpdateFps_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":60}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, UpdateFps_InvalidParameter_Negative)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":-1}"), response));
}

TEST_F(FrameRateTest, UpdateFps_BoundaryValue_Zero)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":0}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, OnDisplayFrameRateChanging_Success)
{
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.DisplayFrameRateChange.framerate = (char*)"1920x1080x60";
    
    string receivedFrameRate;
    bool notificationReceived = false;

    if (FrameRateNotification) {
        auto mockNotification = std::make_shared<FrameRateNotificationMock>();
        EXPECT_CALL(*mockNotification, OnDisplayFrameRateChanging(::testing::_))
            .WillOnce(::testing::Invoke([&](const string& frameRate) {
                receivedFrameRate = frameRate;
                notificationReceived = true;
            }));

        auto originalNotification = FrameRateNotification;
        FrameRateNotification = mockNotification.get();

        _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData, sizeof(eventData));

        FrameRateNotification = originalNotification;
    }

    EXPECT_TRUE(notificationReceived);
    EXPECT_EQ("1920x1080x60", receivedFrameRate);
}

TEST_F(FrameRateTest, OnDisplayFrameRateChanging_EmptyFrameRate)
{
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.DisplayFrameRateChange.framerate = (char*)"";
    
    string receivedFrameRate;
    bool notificationReceived = false;

    if (FrameRateNotification) {
        auto mockNotification = std::make_shared<FrameRateNotificationMock>();
        EXPECT_CALL(*mockNotification, OnDisplayFrameRateChanging(::testing::_))
            .WillOnce(::testing::Invoke([&](const string& frameRate) {
                receivedFrameRate = frameRate;
                notificationReceived = true;
            }));

        auto originalNotification = FrameRateNotification;
        FrameRateNotification = mockNotification.get();

        _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData, sizeof(eventData));

        FrameRateNotification = originalNotification;
    }

    EXPECT_TRUE(notificationReceived);
    EXPECT_EQ("", receivedFrameRate);
}

TEST_F(FrameRateTest, OnDisplayFrameRateChanging_NullFrameRate)
{
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.DisplayFrameRateChange.framerate = nullptr;
    
    string receivedFrameRate;
    bool notificationReceived = false;

    if (FrameRateNotification) {
        auto mockNotification = std::make_shared<FrameRateNotificationMock>();
        EXPECT_CALL(*mockNotification, OnDisplayFrameRateChanging(::testing::_))
            .WillOnce(::testing::Invoke([&](const string& frameRate) {
                receivedFrameRate = frameRate;
                notificationReceived = true;
            }));

        auto originalNotification = FrameRateNotification;
        FrameRateNotification = mockNotification.get();

        _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData, sizeof(eventData));

        FrameRateNotification = originalNotification;
    }

    EXPECT_TRUE(notificationReceived);
    EXPECT_EQ("", receivedFrameRate);
}

TEST_F(FrameRateTest, OnDisplayFrameRateChanged_Success)
{
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.DisplayFrameRateChange.framerate = (char*)"1920x1080x30";
    
    string receivedFrameRate;
    bool notificationReceived = false;

    if (FrameRateNotification) {
        auto mockNotification = std::make_shared<FrameRateNotificationMock>();
        EXPECT_CALL(*mockNotification, OnDisplayFrameRateChanged(::testing::_))
            .WillOnce(::testing::Invoke([&](const string& frameRate) {
                receivedFrameRate = frameRate;
                notificationReceived = true;
            }));

        auto originalNotification = FrameRateNotification;
        FrameRateNotification = mockNotification.get();

        _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, &eventData, sizeof(eventData));

        FrameRateNotification = originalNotification;
    }

    EXPECT_TRUE(notificationReceived);
    EXPECT_EQ("1920x1080x30", receivedFrameRate);
}

TEST_F(FrameRateTest, OnDisplayFrameRateChanged_EmptyFrameRate)
{
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.DisplayFrameRateChange.framerate = (char*)"";
    
    string receivedFrameRate;
    bool notificationReceived = false;

    if (FrameRateNotification) {
        auto mockNotification = std::make_shared<FrameRateNotificationMock>();
        EXPECT_CALL(*mockNotification, OnDisplayFrameRateChanged(::testing::_))
            .WillOnce(::testing::Invoke([&](const string& frameRate) {
                receivedFrameRate = frameRate;
                notificationReceived = true;
            }));

        auto originalNotification = FrameRateNotification;
        FrameRateNotification = mockNotification.get();

        _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, &eventData, sizeof(eventData));

        FrameRateNotification = originalNotification;
    }

    EXPECT_TRUE(notificationReceived);
    EXPECT_EQ("", receivedFrameRate);
}

TEST_F(FrameRateTest, OnDisplayFrameRateChanged_NullFrameRate)
{
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.DisplayFrameRateChange.framerate = nullptr;
    
    string receivedFrameRate;
    bool notificationReceived = false;

    if (FrameRateNotification) {
        auto mockNotification = std::make_shared<FrameRateNotificationMock>();
        EXPECT_CALL(*mockNotification, OnDisplayFrameRateChanged(::testing::_))
            .WillOnce(::testing::Invoke([&](const string& frameRate) {
                receivedFrameRate = frameRate;
                notificationReceived = true;
            }));

        auto originalNotification = FrameRateNotification;
        FrameRateNotification = mockNotification.get();

        _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, &eventData, sizeof(eventData));

        FrameRateNotification = originalNotification;
    }

    EXPECT_TRUE(notificationReceived);
    EXPECT_EQ("", receivedFrameRate);
}

TEST_F(FrameRateTest, OnDisplayFrameRateChanging_HighFrameRate)
{
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.DisplayFrameRateChange.framerate = (char*)"3840x2160x120";
    
    string receivedFrameRate;
    bool notificationReceived = false;

    if (FrameRateNotification) {
        auto mockNotification = std::make_shared<FrameRateNotificationMock>();
        EXPECT_CALL(*mockNotification, OnDisplayFrameRateChanging(::testing::_))
            .WillOnce(::testing::Invoke([&](const string& frameRate) {
                receivedFrameRate = frameRate;
                notificationReceived = true;
            }));

        auto originalNotification = FrameRateNotification;
        FrameRateNotification = mockNotification.get();

        _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, &eventData, sizeof(eventData));

        FrameRateNotification = originalNotification;
    }

    EXPECT_TRUE(notificationReceived);
    EXPECT_EQ("3840x2160x120", receivedFrameRate);
}

TEST_F(FrameRateTest, OnDisplayFrameRateChanged_HighFrameRate)
{
    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.DisplayFrameRateChange.framerate = (char*)"3840x2160x120";
    
    string receivedFrameRate;
    bool notificationReceived = false;

    if (FrameRateNotification) {
        auto mockNotification = std::make_shared<FrameRateNotificationMock>();
        EXPECT_CALL(*mockNotification, OnDisplayFrameRateChanged(::testing::_))
            .WillOnce(::testing::Invoke([&](const string& frameRate) {
                receivedFrameRate = frameRate;
                notificationReceived = true;
            }));

        auto originalNotification = FrameRateNotification;
        FrameRateNotification = mockNotification.get();

        _iarmDSFramerateEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, &eventData, sizeof(eventData));

        FrameRateNotification = originalNotification;
    }

    EXPECT_TRUE(notificationReceived);
    EXPECT_EQ("3840x2160x120", receivedFrameRate);
}
