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

#include "Implementation/DisplayInfo.h"

#include "IarmBusMock.h"

#include <fstream>
#include "ThunderPortability.h"

#include "DisplayInfoMock.h"
#include "AudioOutputPortMock.h"
#include "HostMock.h"
#include "IarmBusMock.h"
#include "ManagerMock.h"
#include "ServiceMock.h"
#include "VideoOutputPortConfigMock.h"
#include "VideoOutputPortMock.h"
#include "VideoOutputPortTypeMock.h"
#include "VideoResolutionMock.h"


#include "SystemInfo.h"

#include <fstream>
#include "ThunderPortability.h"

using namespace WPEFramework;

using ::testing::NiceMock;

namespace {
const string webPrefix = _T("/Service/DisplayInfo");
}

using ::testing::NiceMock;

class DisplayInfoTest : public ::testing::Test {
protected:
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;
    Core::ProxyType<Plugin::IGraphicsProperties> graphicsPropertiesImplementation;
    Core::ProxyType<Plugin::IConnectionProperties> connectionPropertiesImplementation;
    Core::ProxyType<Plugin::IHDRProperties> hdrPropertiesImplementation;
    Core::ProxyType<Plugin::IDisplayProperties> displayPropertiesImplementation;
    Exchange::IGraphicsProperties* graphicsInterface = nullptr;
    Exchange::IConnectionProperties* connectionInterface = nullptr;
    Exchange::IHDRProperties* hdrInterface = nullptr;
    Exchange::IDisplayProperties* displayInterface = nullptr;

    DisplayInfoTest()
    {
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        graphicsPropertiesImplementation = Core::ProxyType<Plugin::IGraphicsProperties>::Create();
        connectionPropertiesImplementation = Core::ProxyType<Plugin::IConnectionProperties>::Create();
        hdrPropertiesImplementation = Core::ProxyType<Plugin::IHDRProperties>::Create();
        displayPropertiesImplementation = Core::ProxyType<Plugin::IDisplayProperties>::Create();

        graphicsInterface = static_cast<Exchange::IGraphicsProperties*>(
            graphicsPropertiesImplementation->QueryInterface(Exchange::IGraphicsProperties::ID));

        connectionInterface = static_cast<Exchange::IConnectionProperties*>(
            connectionPropertiesImplementation->QueryInterface(Exchange::IConnectionProperties::ID));

        hdrInterface = static_cast<Exchange::IHDRProperties*>(
            hdrPropertiesImplementation->QueryInterface(Exchange::IHDRProperties::ID));

        displayInterface = static_cast<Exchange::IDisplayProperties*>(
            displayPropertiesImplementation->QueryInterface(Exchange::IDisplayProperties::ID));

    }
    virtual ~DisplayInfoTest()
    {
        graphicsInterface->Release();
        connectionInterface->Release();
        hdrInterface->Release();
        displayInterface->Release();
        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr)
        {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
    }

    TEST_F(DisplayInfoTest, TotalGpuRam)
    {
        uint64_t total = 0;
        EXPECT_EQ(Core::ERROR_NONE, graphicsInterface->TotalGpuRam(total));
        EXPECT_EQ(total, 4096);
    }
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

