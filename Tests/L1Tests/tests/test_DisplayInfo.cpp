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

    TEST_F(DisplayInfoTestTest, TotalGpuRam)
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

    }
    /*
    TEST_F(DisplayInfoTest, FreeGpuRam)
    {
        uint64_t free = 0;
        EXPECT_EQ(Core::ERROR_NONE, graphicsInterface->FreeGpuRam(free));
        EXPECT_EQ(free, 2048);
    }

    TEST_F(DisplayInfoTest, IsAudioPassthrough)
    {
        bool passthru = false;
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->IsAudioPassthrough(passthru));
        EXPECT_TRUE(passthru);
    }
    TEST_F(DisplayInfoTest, Connected)
    {
        bool connected = false;
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->Connected(connected));
        EXPECT_TRUE(connected);
    }

    TEST_F(DisplayInfoTest, Width)
    {
        uint32_t width = 0;
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->Width(width));
        EXPECT_EQ(width, 1920);
    }
    
    TEST_F(DisplayInfoTest, Height)
    {
        uint32_t height = 0;
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->Height(height));
        EXPECT_EQ(height, 1080);
    }
    
    TEST_F(DisplayInfoTest, VerticalFreq)
    {
        uint32_t vf = 0;
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->VerticalFreq(vf));
        EXPECT_EQ(vf, 60);
    }
    
    TEST_F(DisplayInfoTest, EDID)
    {
        uint16_t length = 4;
        uint8_t data[4] = {};
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->EDID(length, data));
        EXPECT_EQ(length, 4);
        for(int i=0;i<4;i++) EXPECT_EQ(data[i], i+1);
    }
    
    TEST_F(DisplayInfoTest, WidthInCentimeters)
    {
        uint8_t width = 0;
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->WidthInCentimeters(width));
        EXPECT_EQ(width, 100);
    }
    
    TEST_F(DisplayInfoTest, HeightInCentimeters)
    {
        uint8_t height = 0;
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->HeightInCentimeters(height));
        EXPECT_EQ(height, 56);
    }


    TEST_F(DisplayInfoTest, HDCPProtectionGet)
    {
        Exchange::IConnectionProperties::HDCPProtectionType value = Exchange::IConnectionProperties::HDCP_Unencrypted;
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->HDCPProtection(value));
        EXPECT_EQ(value, Exchange::IConnectionProperties::HDCP_2X);
    }
    
    TEST_F(DisplayInfoTest, HDCPProtectionSet)
    {
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->HDCPProtection(Exchange::IConnectionProperties::HDCP_2X));
    }
    
    TEST_F(DisplayInfoTest, PortName)
    {
        std::string name;
        EXPECT_EQ(Core::ERROR_NONE, connectionInterface->PortName(name));
        EXPECT_EQ(name, "HDMI0");
    }

    TEST_F(DisplayInfoTest, TVCapabilities)
    {
        Exchange::IHDRProperties::IHDRIterator* type = nullptr;
        EXPECT_EQ(Core::ERROR_NONE, hdrInterface->TVCapabilities(type));
        EXPECT_EQ(type, nullptr); // Adjust as needed for your mock/implementation
    }
    
    TEST_F(DisplayInfoTest, STBCapabilities)
    {
        Exchange::IHDRProperties::IHDRIterator* type = nullptr;
        EXPECT_EQ(Core::ERROR_NONE, hdrInterface->STBCapabilities(type));
        EXPECT_EQ(type, nullptr); // Adjust as needed for your mock/implementation
    }
    
    TEST_F(DisplayInfoTest, HDRSetting)
    {
        Exchange::IHDRProperties::HDRType type = Exchange::IHDRProperties::HDR_OFF;
        EXPECT_EQ(Core::ERROR_NONE, hdrInterface->HDRSetting(type));
        EXPECT_EQ(type, Exchange::IHDRProperties::HDR_10); // Adjust as needed for your mock/implementation
    }
    
    TEST_F(DisplayInfoTest, ColorSpace)
    {
        Exchange::IDisplayProperties::ColourSpaceType cs = Exchange::IDisplayProperties::FORMAT_UNKNOWN;
        EXPECT_EQ(Core::ERROR_NONE, displayInterface->ColorSpace(cs));
        EXPECT_EQ(cs, Exchange::IDisplayProperties::FORMAT_RGB_444);
    }
    
    TEST_F(DisplayInfoTest, FrameRate)
    {
        Exchange::IDisplayProperties::FrameRateType rate = Exchange::IDisplayProperties::FRAMERATE_UNKNOWN;
        EXPECT_EQ(Core::ERROR_NONE, displayInterface->FrameRate(rate));
        EXPECT_EQ(rate, Exchange::IDisplayProperties::FRAMERATE_60);
    }
    
    TEST_F(DisplayInfoTest, ColourDepth)
    {
        Exchange::IDisplayProperties::ColourDepthType colour = Exchange::IDisplayProperties::COLORDEPTH_UNKNOWN;
        EXPECT_EQ(Core::ERROR_NONE, displayInterface->ColourDepth(colour));
        EXPECT_EQ(colour, Exchange::IDisplayProperties::COLORDEPTH_10_BIT);
    }
    
    TEST_F(DisplayInfoTest, Colorimetry)
    {
        Exchange::IDisplayProperties::IColorimetryIterator* colorimetry = nullptr;
        EXPECT_EQ(Core::ERROR_NONE, displayInterface->Colorimetry(colorimetry));
        EXPECT_EQ(colorimetry, nullptr); // Adjust as needed for your mock/implementation
    }
    
    TEST_F(DisplayInfoTest, QuantizationRange)
    {
        Exchange::IDisplayProperties::QuantizationRangeType qr = Exchange::IDisplayProperties::QUANTIZATIONRANGE_UNKNOWN;
        EXPECT_EQ(Core::ERROR_NONE, displayInterface->QuantizationRange(qr));
        EXPECT_EQ(qr, Exchange::IDisplayProperties::QUANTIZATIONRANGE_FULL);
    }
    
    TEST_F(DisplayInfoTest, EOTF)
    {
        Exchange::IDisplayProperties::EotfType eotf = Exchange::IDisplayProperties::EOTF_UNKNOWN;
        EXPECT_EQ(Core::ERROR_NONE, displayInterface->EOTF(eotf));
        EXPECT_EQ(eotf, Exchange::IDisplayProperties::EOTF_BT2100);
    }
*/
