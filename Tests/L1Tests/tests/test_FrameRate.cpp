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
#include "ManagerMock.h"
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
    IVideoDeviceEventsImplMock   *p_ivideoDeviceMock = nullptr;
    IARM_EventHandler_t _iarmDSFramerateEventHandler;
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;
    ManagerImplMock   *p_managerImplMock = nullptr ;

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

        p_managerImplMock  = new NiceMock <ManagerImplMock>;
        device::Manager::setImpl(p_managerImplMock);

        p_hostImplMock  = new NiceMock <HostImplMock>;
        device::Host::setImpl(p_hostImplMock);

        p_ivideoDeviceMock  = new NiceMock <IVideoDeviceEventsImplMock>;
        device::IVideoDeviceEvents::setImpl(p_ivideoDeviceMock);

        EXPECT_CALL(*p_managerImplMock, Initialize())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        PluginHost::IFactories::Assign(&factoriesImplementation);

        ON_CALL(*p_hostImplMock, Register(::testing::Matcher<device::Host::IVideoDeviceEvents*>(::testing::_)))
            .WillByDefault(testing::Return(dsError_t::dsERR_NONE));

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

        Core::IWorkerPool::Assign(&(*workerPool));
            workerPool->Run();

        plugin->Initialize(&service);

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

        ON_CALL(*p_hostImplMock, UnRegister(::testing::Matcher<device::Host::IVideoDeviceEvents*>(::testing::_)))
            .WillByDefault(testing::Return(dsError_t::dsERR_NONE));

        device::VideoDevice::setImpl(nullptr);
        if (p_videoDeviceMock != nullptr)
        {
            delete p_videoDeviceMock;
            p_videoDeviceMock = nullptr;
        }

        EXPECT_CALL(*p_managerImplMock, DeInitialize())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        device::Manager::setImpl(nullptr);
        if (p_managerImplMock != nullptr)
        {
            delete p_managerImplMock;
            p_managerImplMock = nullptr;
        }

        device::Host::IVideoDeviceEvents::setImpl(nullptr);
        if (p_ivideoDeviceMock != nullptr)
        {
            delete p_ivideoDeviceMock;
            p_ivideoDeviceMock = nullptr;
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
    std::cout<<"RegisteredMethods_1"<<std::endl;
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setCollectionFrequency")));
    std::cout<<"RegisteredMethods_1_1"<<std::endl;
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("startFpsCollection")));
    std::cout<<"RegisteredMethods_1_2"<<std::endl;
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("stopFpsCollection")));
    std::cout<<"RegisteredMethods_1_3"<<std::endl;
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("updateFps")));
    std::cout<<"RegisteredMethods_1_4"<<std::endl;
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setFrmMode")));
    std::cout<<"RegisteredMethods_1_5"<<std::endl;
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getFrmMode")));
    std::cout<<"RegisteredMethods_1_6"<<std::endl;
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setDisplayFrameRate")));
    std::cout<<"RegisteredMethods_1_7"<<std::endl;
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getDisplayFrameRate")));
    std::cout<<"RegisteredMethods_2"<<std::endl;
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
    //ASSERT_TRUE(_iarmDSFramerateEventHandler != nullptr);
    std::cout<<"onDisplayFrameRateChanging_1"<<std::endl;
    Core::Event onDisplayFrameRateChanging(false, true);
    std::cout<<"onDisplayFrameRateChanging_2"<<std::endl;
    EVENT_SUBSCRIBE(0, _T("onDisplayFrameRateChanging"), _T("org.rdk.FrameRate"), message);	
    std::cout<<"onDisplayFrameRateChanging_3"<<std::endl;
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
		onDisplayFrameRateChanging.SetEvent();
                return Core::ERROR_NONE;
            }));
    std::cout<<"onDisplayFrameRateChanging_4"<<std::endl;

    ON_CALL(*p_ivideoDeviceMock, OnDisplayFrameratePreChange(::testing::_))
            .WillByDefault([](const std::string& frameRate){
                std::cout<<"OnDisplayFrameratePreChange mock method"<<std::endl;
			});

    std::cout<<"onDisplayFrameRateChanging_4_1"<<std::endl;
    FrameRateImplem->OnDisplayFrameratePreChange("3840x2160px48");

    std::cout<<"onDisplayFrameRateChanging_5"<<std::endl;
	//EXPECT_EQ(Core::ERROR_NONE, onDisplayFrameRateChanging.Lock());
    std::cout<<"onDisplayFrameRateChanging_6"<<std::endl;
    EVENT_UNSUBSCRIBE(0, _T("onDisplayFrameRateChanging"), _T("org.rdk.FrameRate"), message);
    std::cout<<"onDisplayFrameRateChanging_7"<<std::endl;
}

TEST_F(FrameRateTest, onDisplayFrameRateChanged)
{
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

    ON_CALL(*p_ivideoDeviceMock, onDisplayFrameRateChanged("3840x2160px48"))
            .WillByDefault([](const std::string& frameRate){
                std::cout<<"onDisplayFrameRateChanged mock method"<<std::endl;
			});

    FrameRateImplem->OnDisplayFrameratePostChange("3840x2160px48");

	//EXPECT_EQ(Core::ERROR_NONE, resetDone.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onDisplayFrameRateChanged"), _T("org.rdk.FrameRate"), message);
}
