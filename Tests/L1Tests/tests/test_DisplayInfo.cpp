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
#include "VideoResolutionMock.h"
#include "DrmMock.h"

#include "FactoriesImplementation.h"
#include "HostMock.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "VideoDeviceMock.h"
#include "devicesettings.h"
#include "DisplayMock.h"
#include "EdidParserMock.h"
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
    VideoResolutionMock      *p_videoResolutionMock = nullptr ;
    VideoDeviceMock      *p_videoDeviceMock = nullptr ;
    DisplayMock      *p_displayMock = nullptr ;
    EdidParserMock  *p_edidParserMock = nullptr;
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

        p_displayMock  = new NiceMock <DisplayMock>;
        device::Display::setImpl(p_displayMock);

        p_edidParserMock  = new NiceMock <EdidParserMock>;
        edid_parser::edidParserImpl::setImpl(p_edidParserMock);

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

        p_videoResolutionMock  = new NiceMock <VideoResolutionMock>;
        device::VideoResolution::setImpl(p_videoResolutionMock);

        p_videoOutputPortMock  = new NiceMock <VideoOutputPortMock>;
        device::VideoOutputPort::setImpl(p_videoOutputPortMock);

        p_videoDeviceMock = new NiceMock <VideoDeviceMock>;
        device::VideoDevice::setImpl(p_videoDeviceMock);

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

        device::Display::setImpl(nullptr);
        if (p_displayMock != nullptr)
        {
            delete p_displayMock;
            p_displayMock = nullptr;
        }  

        edid_parser::edidParserImpl::setImpl(nullptr);
        if (p_edidParserMock != nullptr)
        {
            delete p_edidParserMock;
            p_edidParserMock = nullptr;
        }

        device::VideoResolution::setImpl(nullptr);
        if (p_videoResolutionMock != nullptr)
        {
            delete p_videoResolutionMock;
            p_videoResolutionMock = nullptr;
        }  

        device::VideoDevice::setImpl(nullptr);
        if (p_videoDeviceMock != nullptr)
        {
            delete p_videoDeviceMock;
            p_videoDeviceMock = nullptr;
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

    TEST_F(DisplayInfoTestTest, Info_AllProperties)
    {
        // Arrange: Set up device/host mocks to control the environment

        // Video port and display connection
        device::VideoOutputPort videoOutputPort;
        device::AudioOutputPort audioOutputPort;
        device::VideoOutputPortType videoOutputPortType(dsVIDEOPORT_TYPE_HDMI);
        device::Display display;
        std::string videoName = "HDMI-1";
        string videoPort(_T("HDMI0"));

        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoName));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*p_videoOutputPortMock, getAudioOutputPort())
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        ON_CALL(*p_videoOutputPortMock, getDisplay())
            .WillByDefault(::testing::ReturnRef(display));

        // Audio passthrough
        ON_CALL(*p_audioOutputPortMock, getStereoMode(::testing::_))
            .WillByDefault(::testing::Return(device::AudioStereoMode::kPassThru));

        // EDID for width/height
        ON_CALL(*p_displayMock, getEDIDBytes(::testing::_))
            .WillByDefault(::testing::Invoke(
                [](std::vector<uint8_t>& edidVec) {
                    edidVec = std::vector<uint8_t>(23, 0);
                    edidVec[21] = 77; // width in cm
                    edidVec[22] = 55; // height in cm
                }));

        // DRM for GPU RAM
        uint64_t expectedTotalGpuRam = 4096;
        uint64_t expectedFreeGpuRam = 2048;
        EXPECT_CALL(*p_drmMock, drmModeGetResources(::testing::_))
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Invoke([](int) {
                drmModeRes* res = (drmModeRes*)calloc(1, sizeof(drmModeRes));
                res->count_connectors = 1;
                res->count_encoders = 1;
                res->connectors = (uint32_t*)calloc(1, sizeof(uint32_t));
                res->encoders = (uint32_t*)calloc(1, sizeof(uint32_t));
                res->connectors[0] = 123;
                res->encoders[0] = 456;
                return res;
            }));
        EXPECT_CALL(*p_drmMock, drmModeGetConnector(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Invoke([](int, uint32_t connector_id) {
                drmModeConnector* connector = static_cast<drmModeConnector*>(calloc(1, sizeof(*connector)));
                connector->connector_id = connector_id;
                connector->count_modes = 1;
                connector->connection = DRM_MODE_CONNECTED;
                return connector;
            }));
        EXPECT_CALL(*p_drmMock, drmModeGetEncoder(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Invoke([](int, uint32_t encoder_id) {
                drmModeEncoder* encoder = static_cast<drmModeEncoder*>(calloc(1, sizeof(*encoder)));
                encoder->encoder_id = encoder_id;
                encoder->crtc_id = 0;
                encoder->possible_crtcs = 0xFF;
                return encoder;
            }));
        EXPECT_CALL(*p_drmMock, drmModeGetCrtc(::testing::_, ::testing::_))
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Invoke([](int, uint32_t crtc_id) {
                drmModeCrtc* crtc = static_cast<drmModeCrtc*>(calloc(1, sizeof(*crtc)));
                crtc->crtc_id = crtc_id;
                crtc->mode_valid = 1;
                crtc->mode.hdisplay = 1920;
                crtc->mode.vdisplay = 1080;
                return crtc;
            }));
        EXPECT_CALL(*p_drmMock, drmModeFreeEncoder(::testing::A<drmModeEncoderPtr>())).WillRepeatedly(::testing::Invoke([](drmModeEncoderPtr encoder) { free(encoder); }));
        ON_CALL(*p_drmMock, drmModeFreeConnector(::testing::_)).WillByDefault(::testing::Invoke([](drmModeConnectorPtr connector) { free(connector); }));
        ON_CALL(*p_drmMock, drmModeFreeCrtc(::testing::_)).WillByDefault(::testing::Invoke([](drmModeCrtcPtr crtc) { free(crtc); }));
        ON_CALL(*p_drmMock, drmModeFreeResources(::testing::_)).WillByDefault(::testing::Invoke([](drmModeResPtr res) {
            if (res) {
                free(res->connectors);
                free(res);
            }
        }));

        EXPECT_CALL(*p_drmMock, drmModeGetPlane(::testing::_, ::testing::_))
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Invoke([](int fd, uint32_t plane_id) {
                drmModePlanePtr plane = static_cast<drmModePlanePtr>(calloc(1, sizeof(*plane)));
                plane->plane_id = plane_id;
                plane->possible_crtcs = 0xFF;
                plane->crtc_id = 0;
                plane->fb_id = 0;
                plane->crtc_x = 0;
                plane->crtc_y = 0;
                plane->x = 0;
                plane->y = 0;
                plane->gamma_size = 0;
                plane->count_formats = 1;
                plane->formats = (uint32_t*)calloc(1, sizeof(uint32_t));
                plane->formats[0] = DRM_FORMAT_XRGB8888; // Example format
                return plane;
            }));

        EXPECT_CALL(*p_drmMock, drmModeGetFB(::testing::_, ::testing::_))
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Invoke([](int fd, uint32_t buffer_id) {
                drmModeFBPtr fb = static_cast<drmModeFBPtr>(calloc(1, sizeof(*fb)));
                fb->fb_id = buffer_id;
                fb->width = 1920;
                fb->height = 1080;
                fb->pitch = 1920 * 4; // Assuming 32-bit RGBA
                fb->bpp = 32;
                fb->depth = 24;
                fb->handle = 123; // Some dummy handle
                return fb;
            }));

        ON_CALL(*p_drmMock, drmModeFreeFB(::testing::_))
            .WillByDefault(::testing::Invoke([](drmModeFBPtr fb) {
                if (fb) {
                    free(fb);
                }
            }));
        

        // Act: Call the Info method via HTTP GET
        Core::ProxyType<Web::Response> ret(PluginHost::IFactories::Instance().Response());
        Web::Request request;
        request.Verb = Web::Request::HTTP_GET;
        ret = plugin->Process(request);

        // Assert: Check HTTP status
        EXPECT_EQ(Web::STATUS_OK, ret->ErrorCode);

        // Extract and check JSON body
        //Core::ProxyType<Web::IBody> jsonBody = dynamic_cast<Web::JSONBodyType<JsonData::DisplayInfo::DisplayinfoData>*>(ret->Body(Core::ProxyType<Web::IBody>));
        //ASSERT_TRUE(jsonBody != nullptr);
//
        //// Graphics Properties
        //EXPECT_EQ(jsonBody->Width, 1920u);
        //EXPECT_EQ(jsonBody->Height, 1080u);
        //EXPECT_EQ(jsonBody->Totalgpuram, expectedTotalGpuRam);
        //EXPECT_EQ(jsonBody->Freegpuram, expectedFreeGpuRam);
//
        //// Connection Properties
        //EXPECT_EQ(jsonBody->Audiopassthrough, true);
        //EXPECT_EQ(jsonBody->Connected, true);
//
        //// EDID-based width/height in cm
        //EXPECT_EQ(jsonBody->Widthcm, 77u);
        //EXPECT_EQ(jsonBody->Heightcm, 55u);
    }

     TEST_F(DisplayInfoTestTest, VerticalFrequency){
        device::VideoOutputPort videoOutputPort;
        device::AudioOutputPort audioOutputPort;
        device::Display display;

        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));

        ON_CALL(*p_videoOutputPortMock, getDisplay())
            .WillByDefault(::testing::ReturnRef(display));

        ON_CALL(*p_videoOutputPortMock, getAudioOutputPort())
            .WillByDefault(::testing::ReturnRef(audioOutputPort));

        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));

        ON_CALL(*p_displayMock, getEDIDBytes(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](std::vector<uint8_t> &edidVec2) {
                    edidVec2 = std::vector<uint8_t>({ 't', 'e', 's', 't' });
                }));
                
        ON_CALL(*p_edidParserMock, EDID_Verify(::testing::_,::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](unsigned char* bytes, size_t count) {
                    // Mocked verification logic
                    return edid_parser::EDID_STATUS_OK;
                }));

        ON_CALL(*p_edidParserMock, EDID_Parse(::testing::_,::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](unsigned char* bytes, size_t count, edid_parser::edid_data_t* data_ptr) {
                    // Mocked parsing logic
                    edid_parser::edid_res_t res;
                    res.refresh = 60;
                    data_ptr->res = res; // Set the expected vertical frequency
                    return edid_parser::EDID_STATUS_OK;
                }));
                

        uint32_t _connectionId = 0;
        Exchange::IConnectionProperties* connectionProperties = service.Root<Exchange::IConnectionProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(connectionProperties, nullptr);

        uint32_t verticalFreq = 0;
        uint32_t result = connectionProperties->VerticalFreq(verticalFreq);

        uint32_t freqCheck = 60;

        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(verticalFreq, freqCheck); // Change 60 to the expected value for your test/mocks

        // Release the interface after use
        connectionProperties->Release();
     }

    TEST_F(DisplayInfoTestTest, EDID)
    {
        device::VideoOutputPort videoOutputPort;
        device::AudioOutputPort audioOutputPort;
        device::Display display;
        // Arrange: Set up mocks for display connection and EDID bytes
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*p_videoOutputPortMock, getDisplay())
            .WillByDefault(::testing::ReturnRef(display));
        ON_CALL(*p_displayMock, getEDIDBytes(::testing::_))
            .WillByDefault(::testing::Invoke(
                [](std::vector<uint8_t>& edidVec) {
                    edidVec = std::vector<uint8_t>({'A', 'B', 'C', 'D'});
                }));

        // Act: Call the EDID function via the COMRPC interface
        uint32_t _connectionId = 0;
        Exchange::IConnectionProperties* connectionProperties = service.Root<Exchange::IConnectionProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(connectionProperties, nullptr);

        uint16_t length = 4;
        uint8_t data[4] = {};
        uint32_t result = connectionProperties->EDID(length, data);

        // Assert: Check the result and the output buffer
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(length, 4);
        EXPECT_EQ(data[0], 'A');
        EXPECT_EQ(data[1], 'B');
        EXPECT_EQ(data[2], 'C');
        EXPECT_EQ(data[3], 'D');

        connectionProperties->Release();
    }

        TEST_F(DisplayInfoTestTest, WidthInCentimeters)
    {
        device::VideoOutputPort videoOutputPort;
        device::AudioOutputPort audioOutputPort;
        string videoPort(_T("HDMI0"));
        device::Display display;
        // Arrange: Set up mocks for display connection and EDID bytes
        //ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        //    .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*p_videoOutputPortMock, getDisplay())
            .WillByDefault(::testing::ReturnRef(display));
        ON_CALL(*p_displayMock, getEDIDBytes(::testing::_))
            .WillByDefault(::testing::Invoke(
                [](std::vector<uint8_t>& edidVec) {
                    // EDID must be at least 23 bytes for index 21 (EDID_MAX_HORIZONTAL_SIZE)
                    edidVec = std::vector<uint8_t>(23, 0);
                    edidVec[21] = 77; // Set horizontal size in cm at index 21
                }));
    
        // Act: Call the WidthInCentimeters function via the COMRPC interface
        uint32_t _connectionId = 0;
        Exchange::IConnectionProperties* connectionProperties = service.Root<Exchange::IConnectionProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(connectionProperties, nullptr);
    
        uint8_t width = 0;
        uint32_t result = connectionProperties->WidthInCentimeters(width);
    
        // Assert: Check the result and the output value
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(width, 77); // Should match the value set in the mock
    
        connectionProperties->Release();
    }

        TEST_F(DisplayInfoTestTest, HeightInCentimeters)
    {
        device::VideoOutputPort videoOutputPort;
        device::AudioOutputPort audioOutputPort;
        device::VideoOutputPortType videoOutputPortType(dsVIDEOPORT_TYPE_HDMI);
        device::Display display;
        string videoPort(_T("HDMI0"));
        // Arrange: Set up mocks for display connection and EDID bytes
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*p_videoOutputPortMock, getDisplay())
            .WillByDefault(::testing::ReturnRef(display));
        std::string videoName = "HDMI-1";
        
        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoName));
        ON_CALL(*p_displayMock, getEDIDBytes(::testing::_))
            .WillByDefault(::testing::Invoke(
                [](std::vector<uint8_t>& edidVec) {
                    // EDID must be at least 23 bytes for index 22 (EDID_MAX_VERTICAL_SIZE)
                    edidVec = std::vector<uint8_t>(23, 0);
                    edidVec[22] = 55; // Set vertical size in cm at index 22
                }));
    
        // Act: Call the HeightInCentimeters function via the COMRPC interface
        uint32_t _connectionId = 0;
        Exchange::IConnectionProperties* connectionProperties = service.Root<Exchange::IConnectionProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(connectionProperties, nullptr);
    
        uint8_t height = 0;
        uint32_t result = connectionProperties->HeightInCentimeters(height);
    
        // Assert: Check the result and the output value
        EXPECT_EQ(result, Core::ERROR_NONE);
        EXPECT_EQ(height, 55); // Should match the value set in the mock
    
        connectionProperties->Release();
    }

    TEST_F(DisplayInfoTestTest, ColorSpace)
    {
        device::VideoOutputPort videoOutputPort;
        device::AudioOutputPort audioOutputPort;
        device::VideoOutputPortType videoOutputPortType(dsVIDEOPORT_TYPE_HDMI);
        string videoPort(_T("HDMI0"));
        std::string videoName = "HDMI-1";
        
        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoName));

        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*p_videoOutputPortMock, getColorSpace())
            .WillByDefault(::testing::Return(dsDISPLAY_COLORSPACE_RGB)); // This should map to FORMAT_RGB_444

        // Act: Call the ColorSpace function via the COMRPC interface
        struct {
        int input;
        Exchange::IDisplayProperties::ColourSpaceType expected;
        } testCases[] = {
            {dsDISPLAY_COLORSPACE_RGB,        Exchange::IDisplayProperties::FORMAT_RGB_444},
            {dsDISPLAY_COLORSPACE_YCbCr444,   Exchange::IDisplayProperties::FORMAT_YCBCR_444},
            {dsDISPLAY_COLORSPACE_YCbCr422,   Exchange::IDisplayProperties::FORMAT_YCBCR_422},
            {dsDISPLAY_COLORSPACE_YCbCr420,   Exchange::IDisplayProperties::FORMAT_YCBCR_420},
            {dsDISPLAY_COLORSPACE_AUTO,       Exchange::IDisplayProperties::FORMAT_OTHER},
            {dsDISPLAY_COLORSPACE_UNKNOWN,    Exchange::IDisplayProperties::FORMAT_UNKNOWN}
        };

        uint32_t _connectionId = 0;
        Exchange::IDisplayProperties* displayProperties = service.Root<Exchange::IDisplayProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(displayProperties, nullptr);

        for (const auto& test : testCases) {
            EXPECT_CALL(*p_videoOutputPortMock, getColorSpace())
                .WillOnce(::testing::Return(test.input));

            Exchange::IDisplayProperties::ColourSpaceType cs = Exchange::IDisplayProperties::FORMAT_UNKNOWN;
            uint32_t result = displayProperties->ColorSpace(cs);

            EXPECT_EQ(result, Core::ERROR_NONE);
            EXPECT_EQ(cs, test.expected);

        }
        displayProperties->Release();
    }

    TEST_F(DisplayInfoTestTest, FrameRate)
    {
        device::VideoOutputPort videoOutputPort;
        device::AudioOutputPort audioOutputPort;
        device::VideoResolution videoResolution;
        device::PixelResolution pixelResolution;
        device::VideoOutputPortType videoOutputPortType(dsVIDEOPORT_TYPE_HDMI);
        std::string videoName = "HDMI-1";
        
        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoName));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));

        
    
        // Mock the resolution and frame rate chain
        ON_CALL(*p_videoOutputPortMock, getResolution())
            .WillByDefault(::testing::ReturnRef(videoResolution));
        ON_CALL(*p_videoResolutionMock, getPixelResolution())
            .WillByDefault(::testing::ReturnRef(pixelResolution));
    
        // Act: Call the FrameRate function via the COMRPC interface
        uint32_t _connectionId = 0;
        Exchange::IDisplayProperties* displayProperties = service.Root<Exchange::IDisplayProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(displayProperties, nullptr);
    
        struct {
            device::FrameRate frames;
            Exchange::IDisplayProperties::FrameRateType expected;
        } testCases[] = {
            {device::FrameRate(dsVIDEO_FRAMERATE_23dot98), Exchange::IDisplayProperties::FRAMERATE_23_976},
            {device::FrameRate(dsVIDEO_FRAMERATE_24),      Exchange::IDisplayProperties::FRAMERATE_24},
            {device::FrameRate(dsVIDEO_FRAMERATE_25),      Exchange::IDisplayProperties::FRAMERATE_25},
            {device::FrameRate(dsVIDEO_FRAMERATE_29dot97), Exchange::IDisplayProperties::FRAMERATE_29_97},
            {device::FrameRate(dsVIDEO_FRAMERATE_30),      Exchange::IDisplayProperties::FRAMERATE_30},
            {device::FrameRate(dsVIDEO_FRAMERATE_50),      Exchange::IDisplayProperties::FRAMERATE_50},
            {device::FrameRate(dsVIDEO_FRAMERATE_59dot94), Exchange::IDisplayProperties::FRAMERATE_59_94},
            {device::FrameRate(dsVIDEO_FRAMERATE_60),      Exchange::IDisplayProperties::FRAMERATE_60},
            // Add more if your device::FrameRate enum supports them
        };
        
        for (const auto& test : testCases) {
            EXPECT_CALL(*p_videoResolutionMock, getFrameRate())
                .WillOnce(::testing::ReturnRef(test.frames));

            // Act: Call the FrameRate function via the COMRPC interface

            Exchange::IDisplayProperties::FrameRateType rate = Exchange::IDisplayProperties::FRAMERATE_UNKNOWN;
            uint32_t result = displayProperties->FrameRate(rate);

            // Assert: Check the result and the output value
            EXPECT_EQ(result, Core::ERROR_NONE);
            EXPECT_EQ(rate, test.expected);
        }
    
        displayProperties->Release();
    }

        TEST_F(DisplayInfoTestTest, ColourDepth)
    {
        device::VideoOutputPort videoOutputPort;
        device::VideoOutputPortType videoOutputPortType(dsVIDEOPORT_TYPE_HDMI);
        string videoPort(_T("HDMI0"));
        std::string videoName = "HDMI-1";
        
        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoName));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
    
        struct {
            int mockDepth;
            Exchange::IDisplayProperties::ColourDepthType expected;
        } testCases[] = {
            {8,  Exchange::IDisplayProperties::COLORDEPTH_8_BIT},
            {10, Exchange::IDisplayProperties::COLORDEPTH_10_BIT},
            {12, Exchange::IDisplayProperties::COLORDEPTH_12_BIT},
            {0,  Exchange::IDisplayProperties::COLORDEPTH_UNKNOWN}
        };
    
        for (const auto& test : testCases) {
            EXPECT_CALL(*p_videoOutputPortMock, getColorDepth())
                .WillOnce(::testing::Return(test.mockDepth));
    
            uint32_t _connectionId = 0;
            Exchange::IDisplayProperties* displayProperties = service.Root<Exchange::IDisplayProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
            ASSERT_NE(displayProperties, nullptr);
    
            Exchange::IDisplayProperties::ColourDepthType colour = Exchange::IDisplayProperties::COLORDEPTH_UNKNOWN;
            uint32_t result = displayProperties->ColourDepth(colour);
    
            EXPECT_EQ(result, Core::ERROR_NONE);
            EXPECT_EQ(colour, test.expected);
    
            displayProperties->Release();
        }
    }

        TEST_F(DisplayInfoTestTest, QuantizationRange)
    {
        device::VideoOutputPort videoOutputPort;
        device::VideoOutputPortType videoOutputPortType(dsVIDEOPORT_TYPE_HDMI);
        string videoPort(_T("HDMI0"));
        std::string videoName = "HDMI-1";
        
        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoName));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
    
        struct {
            int mockRange;
            Exchange::IDisplayProperties::QuantizationRangeType expected;
        } testCases[] = {
            {dsDISPLAY_QUANTIZATIONRANGE_LIMITED, Exchange::IDisplayProperties::QUANTIZATIONRANGE_LIMITED},
            {dsDISPLAY_QUANTIZATIONRANGE_FULL, Exchange::IDisplayProperties::QUANTIZATIONRANGE_FULL},
            {dsDISPLAY_QUANTIZATIONRANGE_UNKNOWN, Exchange::IDisplayProperties::QUANTIZATIONRANGE_UNKNOWN}
        };
    
        for (const auto& test : testCases) {
            EXPECT_CALL(*p_videoOutputPortMock, getQuantizationRange())
                .WillOnce(::testing::Return(test.mockRange));
    
            uint32_t _connectionId = 0;
            Exchange::IDisplayProperties* displayProperties = service.Root<Exchange::IDisplayProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
            ASSERT_NE(displayProperties, nullptr);
    
            Exchange::IDisplayProperties::QuantizationRangeType qr = Exchange::IDisplayProperties::QUANTIZATIONRANGE_UNKNOWN;
            uint32_t result = displayProperties->QuantizationRange(qr);
    
            EXPECT_EQ(result, Core::ERROR_NONE);
            EXPECT_EQ(qr, test.expected);
    
            displayProperties->Release();
        }
    }
        TEST_F(DisplayInfoTestTest, Colorimetry)
    {
        device::VideoOutputPort videoOutputPort;
        device::Display display;
        string videoPort(_T("HDMI0"));
    
        // Arrange: Set up mocks for display connection and EDID parsing
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*p_videoOutputPortMock, getDisplay())
            .WillByDefault(::testing::ReturnRef(display));
        ON_CALL(*p_displayMock, getEDIDBytes(::testing::_))
            .WillByDefault(::testing::Invoke(
                [](std::vector<uint8_t>& edidVec) {
                    edidVec = std::vector<uint8_t>(128, 0xAA); // Just a dummy EDID
                }));
    
        // Mock EDID_Verify to succeed
        ON_CALL(*p_edidParserMock, EDID_Verify(::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(edid_parser::EDID_STATUS_OK));
    
        // Mock EDID_Parse to set colorimetry_info
        ON_CALL(*p_edidParserMock, EDID_Parse(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [](unsigned char*, size_t, edid_parser::edid_data_t* data_ptr) {
                    // Set colorimetry_info to include XVYCC601 and BT2020CL
                    data_ptr->colorimetry_info = edid_parser::COLORIMETRY_INFO_XVYCC601 | edid_parser::COLORIMETRY_INFO_XVYCC709 | edid_parser::COLORIMETRY_INFO_SYCC601 | edid_parser::COLORIMETRY_INFO_ADOBEYCC601 | edid_parser::COLORIMETRY_INFO_ADOBERGB | edid_parser::COLORIMETRY_INFO_BT2020CL | edid_parser::COLORIMETRY_INFO_BT2020NCL |edid_parser::COLORIMETRY_INFO_BT2020RGB | edid_parser::COLORIMETRY_INFO_DCI_P3;
                    return edid_parser::EDID_STATUS_OK;
                }));
    
        // Act: Call the Colorimetry function via the COMRPC interface
        uint32_t _connectionId = 0;
        Exchange::IDisplayProperties* displayProperties = service.Root<Exchange::IDisplayProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(displayProperties, nullptr);
    
        Exchange::IDisplayProperties::IColorimetryIterator* colorimetry = nullptr;
        uint32_t result = displayProperties->Colorimetry(colorimetry);
    
        // Assert: Check the result and the output values
        EXPECT_EQ(result, Core::ERROR_NONE);
        ASSERT_NE(colorimetry, nullptr);

        // Should contain COLORIMETRY_XVYCC601 and COLORIMETRY_BT2020YCCBCBRC
        std::vector<Exchange::IDisplayProperties::ColorimetryType> expected = {
            Exchange::IDisplayProperties::COLORIMETRY_XVYCC601,
            Exchange::IDisplayProperties::COLORIMETRY_XVYCC709,
            Exchange::IDisplayProperties::COLORIMETRY_SYCC601,
            Exchange::IDisplayProperties::COLORIMETRY_OPYCC601,
            Exchange::IDisplayProperties::COLORIMETRY_OPRGB,
            Exchange::IDisplayProperties::COLORIMETRY_BT2020YCCBCBRC,
            Exchange::IDisplayProperties::COLORIMETRY_BT2020RGB_YCBCR,
            Exchange::IDisplayProperties::COLORIMETRY_OTHER // DCI_P3 maps to OTHER
        };
    
        // Collect all returned colorimetry types
        Exchange::IDisplayProperties::ColorimetryType info;
        int count = 0;
        if (colorimetry->IsValid()) {
            EXPECT_EQ(colorimetry->Current(), expected[count]);
            count++;
            while (colorimetry->Next(info)) {

                EXPECT_EQ(colorimetry->Current(), expected[count]);
                count++;
            }
            
        }
    
        colorimetry->Release();
    }

    TEST_F(DisplayInfoTestTest, GetHDCPProtection)
    {
        device::VideoOutputPort videoOutputPort;
        device::VideoOutputPortType videoOutputPortType(dsVIDEOPORT_TYPE_HDMI);
        string videoPort(_T("HDMI0"));
    
        
        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoPort));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
    
        struct {
            int mockHdcpVersion;
            Exchange::IConnectionProperties::HDCPProtectionType expected;
        } testCases[] = {
            {dsHDCP_VERSION_1X, Exchange::IConnectionProperties::HDCP_1X},
            {dsHDCP_VERSION_2X, Exchange::IConnectionProperties::HDCP_2X},
            {dsHDCP_VERSION_MAX, Exchange::IConnectionProperties::HDCP_AUTO}
        };
        uint32_t _connectionId = 0;
        Exchange::IConnectionProperties* connectionProperties = service.Root<Exchange::IConnectionProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(connectionProperties, nullptr);
        for (const auto& test : testCases) {
            EXPECT_CALL(*p_videoOutputPortMock, GetHdmiPreference())
                .WillOnce(::testing::Return(test.mockHdcpVersion));
    
            Exchange::IConnectionProperties::HDCPProtectionType hdcp = Exchange::IConnectionProperties::HDCP_Unencrypted;
            uint32_t result = static_cast<const Exchange::IConnectionProperties*>(connectionProperties)->HDCPProtection(hdcp);
    
            EXPECT_EQ(result, Core::ERROR_NONE);
            EXPECT_EQ(hdcp, test.expected);
    
        }
        connectionProperties->Release();
    }    

    TEST_F(DisplayInfoTestTest, SetHDCPProtection)
    {
        device::VideoOutputPort videoOutputPort;
        device::VideoOutputPortType videoOutputPortType(dsVIDEOPORT_TYPE_HDMI);
        string videoPort(_T("HDMI0"));
    
        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoPort));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
    
        struct {
            Exchange::IConnectionProperties::HDCPProtectionType input;
            dsHdcpProtocolVersion_t expectedVersion;
        } testCases[] = {
            {Exchange::IConnectionProperties::HDCP_1X, dsHDCP_VERSION_1X},
            {Exchange::IConnectionProperties::HDCP_2X, dsHDCP_VERSION_2X},
            {Exchange::IConnectionProperties::HDCP_AUTO, dsHDCP_VERSION_MAX}
        };
    
        uint32_t _connectionId = 0;
        Exchange::IConnectionProperties* connectionProperties = service.Root<Exchange::IConnectionProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(connectionProperties, nullptr);
    
        for (const auto& test : testCases) {
            EXPECT_CALL(*p_videoOutputPortMock, SetHdmiPreference(::testing::_))
            .WillOnce(::testing::Invoke(
                [&](dsHdcpProtocolVersion_t version) {
                    EXPECT_EQ(version, test.expectedVersion); // Validate the value
                    return true;
                }));
    
            uint32_t result = connectionProperties->HDCPProtection(test.input);
    
            EXPECT_EQ(result, Core::ERROR_NONE);
        }
    
        connectionProperties->Release();
    }

    TEST_F(DisplayInfoTestTest, EOTF)
    {
        device::VideoOutputPort videoOutputPort;
        device::VideoOutputPortType videoOutputPortType(dsVIDEOPORT_TYPE_HDMI);
        string videoPort(_T("HDMI0"));
        std::string videoName = "HDMI-1";
    
        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoName));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
    
        struct {
            int input;
            Exchange::IDisplayProperties::EotfType expected;
        } testCases[] = {
            {dsHDRSTANDARD_HDR10, Exchange::IDisplayProperties::EOTF_SMPTE_ST_2084},
            {dsHDRSTANDARD_HLG, Exchange::IDisplayProperties::EOTF_BT2100},
            {999, Exchange::IDisplayProperties::EOTF_UNKNOWN} // test an unhandled value
        };
    
        uint32_t _connectionId = 0;
        Exchange::IDisplayProperties* displayProperties = service.Root<Exchange::IDisplayProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(displayProperties, nullptr);
    
        for (const auto& test : testCases) {
            EXPECT_CALL(*p_videoOutputPortMock, getVideoEOTF())
                .WillOnce(::testing::Return(test.input));
    
            Exchange::IDisplayProperties::EotfType eotf = Exchange::IDisplayProperties::EOTF_UNKNOWN;
            uint32_t result = displayProperties->EOTF(eotf);
    
            EXPECT_EQ(result, Core::ERROR_NONE);
            EXPECT_EQ(eotf, test.expected);
        }
    
        displayProperties->Release();
    }


    TEST_F(DisplayInfoTestTest, TVCapabilities)
    {
        device::VideoOutputPort videoOutputPort;
        string videoPort(_T("HDMI0"));
    
        // Arrange: Set up mocks for display connection and HDR capabilities
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(std::vector<device::VideoOutputPort>({videoOutputPort})));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
    
        struct {
            int mockCapabilities;
            std::vector<Exchange::IHDRProperties::HDRType> expected;
        } testCases[] = {
            {0, {Exchange::IHDRProperties::HDR_OFF}},
            {dsHDRSTANDARD_HDR10, {Exchange::IHDRProperties::HDR_10}},
            {dsHDRSTANDARD_HLG, {Exchange::IHDRProperties::HDR_HLG}},
            {dsHDRSTANDARD_DolbyVision, {Exchange::IHDRProperties::HDR_DOLBYVISION}},
            {dsHDRSTANDARD_TechnicolorPrime, {Exchange::IHDRProperties::HDR_TECHNICOLOR}},
            {dsHDRSTANDARD_SDR, {Exchange::IHDRProperties::HDR_SDR}},
            // Test multiple capabilities combined
            {dsHDRSTANDARD_HDR10 | dsHDRSTANDARD_HLG, {Exchange::IHDRProperties::HDR_10, Exchange::IHDRProperties::HDR_HLG}},
            {dsHDRSTANDARD_HDR10 | dsHDRSTANDARD_HLG | dsHDRSTANDARD_DolbyVision, {Exchange::IHDRProperties::HDR_10, Exchange::IHDRProperties::HDR_HLG, Exchange::IHDRProperties::HDR_DOLBYVISION}},
            // Add more combinations as needed
        };

        uint32_t _connectionId = 0;
        Exchange::IHDRProperties* hdrProperties = service.Root<Exchange::IHDRProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(hdrProperties, nullptr);
    
        for (const auto& test : testCases) {
            EXPECT_CALL(*p_videoOutputPortMock, getTVHDRCapabilities(::testing::_))
                .WillOnce(::testing::Invoke(
                    [&](int* caps) {
                        *caps = test.mockCapabilities;
                    }));
                
            Exchange::IHDRProperties::IHDRIterator* iterator = nullptr;
            uint32_t result = hdrProperties->TVCapabilities(iterator);
                
            EXPECT_EQ(result, Core::ERROR_NONE);
            ASSERT_NE(iterator, nullptr);
                
            Exchange::IHDRProperties::HDRType info;
            int count = 0;
            if (iterator->IsValid()) {
                EXPECT_EQ(iterator->Current(), test.expected[count]);
                count++;
                while (iterator->Next(info)) {
                    EXPECT_EQ(iterator->Current(), test.expected[count]);
                    count++;
                }
            }
            iterator->Release();
        
        }
        hdrProperties->Release();
    }

    TEST_F(DisplayInfoTestTest, STBCapabilities)
    {
        device::VideoDevice videoDevice;
    
        // Arrange: Set up mocks for STB HDR capabilities
        ON_CALL(*p_hostImplMock, getVideoDevices())
            .WillByDefault(::testing::Return(std::vector<device::VideoDevice>({videoDevice})));
    
        struct {
            int mockCapabilities;
            std::vector<Exchange::IHDRProperties::HDRType> expected;
        } testCases[] = {
            {0, {Exchange::IHDRProperties::HDR_OFF}},
            {dsHDRSTANDARD_HDR10, {Exchange::IHDRProperties::HDR_10}},
            {dsHDRSTANDARD_HLG, {Exchange::IHDRProperties::HDR_HLG}},
            {dsHDRSTANDARD_DolbyVision, {Exchange::IHDRProperties::HDR_DOLBYVISION}},
            {dsHDRSTANDARD_TechnicolorPrime, {Exchange::IHDRProperties::HDR_TECHNICOLOR}},
            {dsHDRSTANDARD_Invalid, {Exchange::IHDRProperties::HDR_OFF}},
            // Test multiple capabilities combined
            {dsHDRSTANDARD_HDR10 | dsHDRSTANDARD_HLG, {Exchange::IHDRProperties::HDR_10, Exchange::IHDRProperties::HDR_HLG}},
            {dsHDRSTANDARD_HDR10 | dsHDRSTANDARD_HLG | dsHDRSTANDARD_DolbyVision, {Exchange::IHDRProperties::HDR_10, Exchange::IHDRProperties::HDR_HLG, Exchange::IHDRProperties::HDR_DOLBYVISION}},
            // Add more combinations as needed
        };
    
        uint32_t _connectionId = 0;
        Exchange::IHDRProperties* hdrProperties = service.Root<Exchange::IHDRProperties>(_connectionId, 2000, _T("DisplayInfoImplementation"));
        ASSERT_NE(hdrProperties, nullptr);
    
        for (const auto& test : testCases) {
            EXPECT_CALL(*p_videoDeviceMock, getHDRCapabilities(::testing::_))
                .WillOnce(::testing::Invoke(
                    [&](int* caps) {
                        *caps = test.mockCapabilities;
                    }));
    
            Exchange::IHDRProperties::IHDRIterator* iterator = nullptr;
            uint32_t result = hdrProperties->STBCapabilities(iterator);
    
            EXPECT_EQ(result, Core::ERROR_NONE);
            ASSERT_NE(iterator, nullptr);
    
            std::vector<Exchange::IHDRProperties::HDRType> values;
            Exchange::IHDRProperties::HDRType info;
            int count = 0;
            if (iterator->IsValid()) {
                values.push_back(iterator->Current());
                EXPECT_EQ(iterator->Current(), test.expected[count]);
                count++;
                while (iterator->Next(info)) {
                    EXPECT_EQ(iterator->Current(), test.expected[count]);
                    values.push_back(info);
                    count++;
                }
            }
            iterator->Release();
        }
        hdrProperties->Release();
    }
