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


#include <gtest/gtest.h>

#include "../../../DisplayInfo/DeviceSettings/PlatformImplementation.cpp"
#include "DisplayInfo.h"
#include "DisplayInfoMock.h"

#include "IarmBusMock.h"
#include "ManagerMock.h"

#include <fstream>
#include "ThunderPortability.h"

#include "AudioOutputPortMock.h"
#include "VideoOutputPortConfigMock.h"
#include "VideoOutputPortMock.h"
#include "VideoOutputPortTypeMock.h"
#include "DrmMock.h"

#include "FactoriesImplementation.h"
#include "HostMock.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "VideoDeviceMock.h"
#include "devicesettings.h"
#include "dsMgr.h"
#include "ThunderPortability.h"
#include "WorkerPoolImplementation.h"
#include "WrapsMock.h"

#include "SystemInfo.h"

#include <fstream>
#include <boost/filesystem.hpp>
#include "ThunderPortability.h"

using namespace WPEFramework;

using ::testing::NiceMock;

class DisplayInfoTest : public ::testing::Test {
protected:
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;
    Core::ProxyType<Plugin::DisplayInfo> plugin;
    Core::ProxyType<Plugin::DisplayInfoImplementation> displayInfoImplementation;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    NiceMock<ServiceMock> service;
    PLUGINHOST_DISPATCHER *dispatcher;
    ManagerImplMock   *p_managerImplMock = nullptr ;
    ConnectionPropertiesMock* p_connectionpropertiesMock = nullptr;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    NiceMock<FactoriesImplementation> factoriesImplementation;
    ServiceMock  *p_serviceMock  = nullptr;
    WrapsImplMock* p_wrapsImplMock = nullptr;
    HostImplMock      *p_hostImplMock = nullptr ;
    AudioOutputPortMock      *p_audioOutputPortMock = nullptr ;
    VideoOutputPortMock      *p_videoOutputPortMock = nullptr ;
    DRMMock *p_drmMock = nullptr;
    IARM_EventHandler_t _iarmDisplayInfoPreChangeEventHandler;
    IARM_EventHandler_t _iarmDisplayInfoPowtChangeEventHandler;
    Exchange::IConnectionProperties::INotification *ConnectionProperties = nullptr;

    DisplayInfoTest()
    : plugin(Core::ProxyType<Plugin::DisplayInfo>::Create())
    , handler(*(plugin))
    , INIT_CONX(1, 0)
    , workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
      2, Core::Thread::DefaultStackSize(), 16))
    {
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        p_serviceMock = new NiceMock <ServiceMock>;

        p_connectionpropertiesMock  = new NiceMock <ConnectionPropertiesMock>;



	    p_wrapsImplMock = new NiceMock<WrapsImplMock>;
        Wraps::setImpl(p_wrapsImplMock);

        p_managerImplMock  = new NiceMock <ManagerImplMock>;
        device::Manager::setImpl(p_managerImplMock);

        EXPECT_CALL(*p_managerImplMock, Initialize())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
        plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);
        plugin->Initialize(&service);

        p_hostImplMock  = new NiceMock <HostImplMock>;
        device::Host::setImpl(p_hostImplMock);

        p_drmMock  = new NiceMock <DRMMock>;
        drmImpl::setImpl(p_drmMock);

        p_audioOutputPortMock  = new NiceMock <AudioOutputPortMock>;
        device::AudioOutputPort::setImpl(p_audioOutputPortMock);

        p_videoOutputPortMock  = new NiceMock <VideoOutputPortMock>;
        device::VideoOutputPort::setImpl(p_videoOutputPortMock);

        ON_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_RES_PRECHANGE)) 			{
			            //FrameRatePreChange = handler;
	            		_iarmDisplayInfoPreChangeEventHandler = handler;
                    }
                    if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_RES_POSTCHANGE))			{
			            //FrameRatePostChange = handler;
			            _iarmDisplayInfoPowtChangeEventHandler = handler;
                    }
                    return IARM_RESULT_SUCCESS;
                }));


        //displayInfoImplementation = Core::ProxyType<Plugin::DisplayInfoImplementation>::Create();

        ON_CALL(*p_connectionpropertiesMock, Register(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](Exchange::IConnectionProperties::INotification* notification) {
                    ConnectionProperties = notification;
		    return Core::ERROR_NONE;
                }));


    }
    virtual ~DisplayInfoTest()
    {
        plugin->Deinitialize(&service);
        dispatcher->Deactivate();
        dispatcher->Release();
        

        if (p_serviceMock != nullptr)
        {
            delete p_serviceMock;
            p_serviceMock = nullptr;
        }

        Wraps::setImpl(nullptr);
        if (p_wrapsImplMock != nullptr) {
            delete p_wrapsImplMock;
            p_wrapsImplMock = nullptr;
        }

        drmImpl::setImpl(nullptr);
        if (p_drmMock != nullptr)
        {
            delete p_drmMock;
        }
           
        
        device::Manager::setImpl(nullptr);
        if (p_managerImplMock != nullptr)
        {
            delete p_managerImplMock;
            p_managerImplMock = nullptr;
        }
        if (p_connectionpropertiesMock != nullptr) {
            delete p_connectionpropertiesMock;
            p_connectionpropertiesMock = nullptr;
        }

        device::AudioOutputPort::setImpl(nullptr);
        if (p_audioOutputPortMock != nullptr)
        {
            delete p_audioOutputPortMock;
            p_audioOutputPortMock = nullptr;
        }
        device::VideoOutputPort::setImpl(nullptr);
        if (p_videoOutputPortMock != nullptr)
        {
            delete p_videoOutputPortMock;
            p_videoOutputPortMock = nullptr;
        }

        device::Host::setImpl(nullptr);
        if (p_hostImplMock != nullptr)
        {
            delete p_hostImplMock;
            p_hostImplMock = nullptr;
        }

        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr)
        {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
        
    }
};

class DisplayInfoTestTest : public DisplayInfoTest {
protected:

    DisplayInfoTestTest()
        : DisplayInfoTest()
    {}
        
    virtual ~DisplayInfoTestTest() override
    {

    }
};

    TEST_F(DisplayInfoTestTest, Info)
    {
        device::VideoOutputPort videoOutputPort;
        device::AudioOutputPort audioOutputPort;
        //std::ofstream file("/proc/meminfo");
        //file << "MemTotal: 4096";
        //file.close();
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));

        ON_CALL(*p_videoOutputPortMock, getAudioOutputPort())
            .WillByDefault(::testing::ReturnRef(audioOutputPort));

        ON_CALL(*p_videoOutputPortMock, getAudioOutputPort())
            .WillByDefault(::testing::ReturnRef(audioOutputPort));

        drmModeRes* res = (drmModeRes*)calloc(1, sizeof(drmModeRes));
        res->count_connectors = 1;
        res->connectors = (uint32_t*)calloc(1, sizeof(uint32_t));
        res->connectors[0] = 123; // Example connector ID

        ON_CALL(*p_drmMock, drmModeGetResources(::testing::_))
            .WillByDefault(::testing::Return(res));
        
        ON_CALL(*p_drmMock, drmModeGetPlane(::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](int drm_fd, uint32_t plane_id) {
                    drmModePlanePtr plane = static_cast<drmModePlanePtr>(calloc(1, sizeof(*plane)));
                    plane->fb_id = 1;
                    return plane;
                }));

        ON_CALL(*p_drmMock, drmModeGetConnector(::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [](int drm_fd, uint32_t connector_id) {
                    drmModeConnectorPtr connector = static_cast<drmModeConnectorPtr>(calloc(1, sizeof(*connector)));
                    connector->connector_id = connector_id;
                    connector->count_modes = 1; // Must be >0 for kms_setup_connector
                    connector->connection = DRM_MODE_CONNECTED; // Must be connected
                    return connector;
                }));

        ON_CALL(*p_drmMock, drmModeGetEncoder(::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [](int drm_fd, uint32_t encoder_id) {
                    drmModeEncoderPtr encoder = static_cast<drmModeEncoderPtr>(calloc(1, sizeof(*encoder)));
                    encoder->encoder_id = encoder_id;
                    encoder->crtc_id = 0; // or any valid CRTC id
                    encoder->possible_crtcs = 0xFF; // all CRTCs possible
                    return encoder;
                }));

        ON_CALL(*p_drmMock, drmModeFreeEncoder(::testing::A<drmModeEncoderPtr>()))
            .WillByDefault(::testing::Invoke(
                [](drmModeEncoderPtr encoder) {
                    free(encoder);
                }));

        ON_CALL(*p_drmMock, drmModeGetCrtc(::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [](int drm_fd, uint32_t crtc_id) {
                    drmModeCrtcPtr crtc = static_cast<drmModeCrtcPtr>(calloc(1, sizeof(*crtc)));
                    crtc->crtc_id = crtc_id;
                    crtc->mode_valid = 1; // Must be non-zero for kms_setup_crtc
                    // Optionally set crtc->mode fields if your code uses them
                    return crtc;
                }));


        Core::ProxyType<Web::Response> ret;
        Web::Request request;
        request.Verb = Web::Request::HTTP_GET;
        //JsonData::DisplayInfo::DisplayinfoData& displayInfo;
        ret = plugin->Process(request);

        // Check HTTP status
        EXPECT_EQ(Web::STATUS_OK, ret->ErrorCode);
        //auto jsonBody = dynamic_cast<Web::JSONBodyType<JsonData::DisplayInfo::DisplayinfoData>*>(ret->Body());
        //ASSERT_TRUE(jsonBody.IsValid());
        //
        //// Graphics Properties
        //EXPECT_EQ(jsonBody->Totalgpuram, 2042);
        //EXPECT_EQ(jsonBody->Freegpuram, 1024);
        //
        //// Connection Properties
        //EXPECT_EQ(jsonBody->Audiopassthrough, false);
        //EXPECT_EQ(jsonBody->Connected, false);
        //EXPECT_EQ(jsonBody->Width, 1920);
        //EXPECT_EQ(jsonBody->Height, 1080);
        //
        //// HDCP
        //EXPECT_EQ(jsonBody->Hdcpprotection, Exchange::IConnectionProperties::HDCP_Unencrypted); // or whatever _value is set to
        //
        //// HDR
        //EXPECT_EQ(jsonBody->Hdrtype, Exchange::IHDRProperties::HDR_OFF); // Default in PlatformImplementation.cpp
        

    }

     TEST_F(DisplayInfoTestTest, VerticalFrequency){
        Exchange::IConnectionProperties* connectionProperties = service->Root<Exchange::IConnectionProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(connectionProperties, nullptr);

        uint32_t verticalFreq = 0;
        uint32_t result = connectionProperties->VerticalFreq(verticalFreq);

        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(verticalFreq, 60); // Change 60 to the expected value for your test/mocks

        // Release the interface after use
        connectionProperties->Release();
     }
