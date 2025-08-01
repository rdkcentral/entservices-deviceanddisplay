/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 RDK Management
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
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "L2Tests.h"
#include "L2TestsMock.h"
#include <fstream>
#include "devicesettings.h"
#include "FrontPanelIndicatorMock.h"

#include <mutex>
#include <condition_variable>
 
 
#define JSON_TIMEOUT   (1000)
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);
#define DISPLAYSETTINGS_CALLSIGN  _T("org.rdk.DisplaySettings.1")
#define DISPLAYSETTINGSL2TEST_CALLSIGN _T("L2tests.1")
using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;


class DisplaySettings_L2test : public L2TestMocks {
protected:
    Core::JSONRPC::Message message;
    string response;
    // testing::NiceMock<FrontPanelIndicatorMock> frontPanelIndicatorMock;
    // IARM_EventHandler_t checkAtmosCapsEventHandler = nullptr;
    // IARM_EventHandler_t DisplResolutionHandler = nullptr;
    // IARM_EventHandler_t dsHdmiEventHandler = nullptr;
    // IARM_EventHandler_t formatUpdateEventHandler = nullptr;
    // IARM_EventHandler_t powerEventHandler = nullptr;
    // IARM_EventHandler_t audioPortStateEventHandler = nullptr;
    // IARM_EventHandler_t dsSettingsChangeEventHandler = nullptr;
    // IARM_EventHandler_t ResolutionPostChange = nullptr;

    virtual ~DisplaySettings_L2test() override;
        
    public:
        DisplaySettings_L2test();

};
 
/**
* @brief Constructor for DisplaySettings L2 test class
*/
DisplaySettings_L2test::DisplaySettings_L2test()
        : L2TestMocks()
{
    
        printf("DISPLAYSETTINGS Constructor\n");
        uint32_t status = Core::ERROR_GENERAL;

         /* Activate plugin in constructor */
         status = ActivateService("org.rdk.PowerManager");
         EXPECT_EQ(Core::ERROR_NONE, status);

         status = ActivateService("org.rdk.DisplaySettings");
         EXPECT_EQ(Core::ERROR_NONE, status);
 
}
 
/**
* @brief Destructor for DisplaySettings L2 test class
*/
DisplaySettings_L2test::~DisplaySettings_L2test()
{
    printf("DISPLAYSETTINGS Destructor\n");
    uint32_t status = Core::ERROR_GENERAL;

    status = DeactivateService("org.rdk.DisplaySettings");
    EXPECT_EQ(Core::ERROR_NONE, status);

    status = DeactivateService("org.rdk.PowerManager");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

TEST_F(DisplaySettings_L2test, DisplaySettings_L2_MethodTest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DISPLAYSETTINGS_CALLSIGN, DISPLAYSETTINGSL2TEST_CALLSIGN);
    JsonObject result, params;

    string videoPort(_T("HDMI0"));
    string audioPort(_T("HDMI0"));

    device::AudioOutputPort audioOutputPort;
    device::VideoOutputPort videoOutputPort;
    device::VideoDevice videoDevice;
    device::VideoResolution videoResolution;
    device::VideoOutputPortType videoOutputPortType;
    device::VideoDFC actualVideoDFC;
    string videoDFCName(_T("FULL"));
    string videoPortSupportedResolution(_T("1080p"));

    ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        .WillByDefault(::testing::Return(videoPort));
    ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));

    /*********************DisplaySettings Calls - Start*********************************************/
    device::AudioOutputPort audioFormat;
    ON_CALL(*p_hostImplMock, getCurrentAudioFormat(testing::_))
    .WillByDefault(testing::Invoke(
        [](dsAudioFormat_t audioFormat) {
            audioFormat=dsAUDIO_FORMAT_NONE;
            return 0;
            }));

    // ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
    //     .WillByDefault(::testing::Return(videoPort));

    ON_CALL(*p_videoOutputPortMock, getDefaultResolution())
        .WillByDefault(::testing::ReturnRef(videoResolution));

    ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));

    // ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
    //     .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));

    ON_CALL(*p_videoOutputPortMock, getName())
        .WillByDefault(::testing::ReturnRef(videoPort));

    ON_CALL(*p_audioOutputPortMock, getName())
        .WillByDefault(::testing::ReturnRef(audioPort));

    ON_CALL(*p_videoOutputPortMock, getVideoEOTF())
        .WillByDefault(::testing::Return(1));

    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>({ videoDevice })));

    ON_CALL(*p_videoDeviceMock, getHDRCapabilities(testing::_))
        .WillByDefault([](int* capabilities) {
            if (capabilities) {
                *capabilities = dsHDRSTANDARD_TechnicolorPrime;
            }
        });

    ON_CALL(*p_videoOutputPortMock, getColorDepthCapabilities(testing::_))
        .WillByDefault(testing::Invoke(
            [&](unsigned int* capabilities) {
            *capabilities = dsDISPLAY_COLORDEPTH_8BIT;
        }));

    ON_CALL(*p_videoOutputPortMock, setResolution(testing::_, testing::_, testing::_))
    .WillByDefault(testing::Invoke(
        [&](std::string resolution, bool persist, bool isIgnoreEdid) {
                        printf("Inside setResolution Mock\n");
                        EXPECT_EQ(resolution, "1080p60");
                        EXPECT_EQ(persist, true);

        std::cout << "Resolution: " << resolution 
                << ", Persist: " << persist 
                << ", Ignore EDID: " << isIgnoreEdid << std::endl;
    }));

    ON_CALL(*p_videoOutputPortMock, getCurrentOutputSettings(testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillByDefault([](int& videoEOTF, int& matrixCoefficients, int& colorSpace, 
                        int& colorDepth, int& quantizationRange) {
            videoEOTF = 1;  // example values
            matrixCoefficients = 0;
            colorSpace = 3;
            colorDepth = 10;
            quantizationRange = 4;

            return true;
        });

    ON_CALL(*p_videoOutputPortConfigImplMock, getPortType(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPortType));
    ON_CALL(*p_videoOutputPortMock, getType())
        .WillByDefault(::testing::ReturnRef(videoOutputPortType));
    ON_CALL(*p_videoOutputPortTypeMock, getId())
        .WillByDefault(::testing::Return(0));
    // device::VideoResolution videoResolution;
    ON_CALL(*p_videoOutputPortTypeMock, getSupportedResolutions())
        .WillByDefault(::testing::Return(device::List<device::VideoResolution>({ videoResolution })));
    ON_CALL(*p_videoResolutionMock, getName())
        .WillByDefault(::testing::ReturnRef(videoPortSupportedResolution));

    ON_CALL(*p_videoOutputPortMock, setForceHDRMode(::testing::_))
        .WillByDefault(::testing::Return(true));

    ON_CALL(*p_videoOutputPortMock, getResolution())
    .WillByDefault([]() -> const device::VideoResolution& {
        static device::VideoResolution dynamicResolution;
        return dynamicResolution;
    });

    ON_CALL(*p_videoDeviceMock, getDFC())
        .WillByDefault(::testing::Invoke([&]() -> device::VideoDFC& {
            return actualVideoDFC;
        }));

    ON_CALL(*p_videoDFCMock, getName())
    .WillByDefault(::testing::ReturnRef(videoDFCName));

    ON_CALL(*p_videoOutputPortMock, getTVHDRCapabilities(testing::_))
        .WillByDefault(testing::Invoke(
            [&](int* capabilities) {
            *capabilities = dsHDRSTANDARD_HLG | dsHDRSTANDARD_HDR10;
        }));

     ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(audioOutputPort));

    ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(audioOutputPort));

    ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(audioOutputPort));

    ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(audioOutputPort));

    ON_CALL(*p_audioOutputPortMock, isConnected())
        .WillByDefault(::testing::Return(true));

    ON_CALL(*p_audioOutputPortMock, getBassEnhancer())
        .WillByDefault(::testing::Return(50));

    dsSurroundVirtualizer_t mockVirtualizer;
    mockVirtualizer.mode = 1;
    mockVirtualizer.boost = 90;

    ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(audioOutputPort));

    ON_CALL(*p_audioOutputPortMock, isConnected())
        .WillByDefault(::testing::Return(true));

    ON_CALL(*p_audioOutputPortMock, getSurroundVirtualizer())
        .WillByDefault(::testing::Return(mockVirtualizer));

    ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(audioOutputPort));

    ON_CALL(*p_audioOutputPortMock, isConnected())
        .WillByDefault(::testing::Return(true));

    /*********************DisplaySettings Calls - End*********************************************/

    /**************getCurrentResolution********************/

    {
        JsonObject result, params;
        uint32_t status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getCurrentResolution", params, result);
    }
    /************************getAudioFormat***************************/
    uint32_t status = Core::ERROR_NONE;

    {
        JsonObject result, params;
        uint32_t status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getAudioFormat", params, result);
    }
    /************************getVideoFormat***************************/

    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getVideoFormat", params, result);
    }

    /****************getColorDepthCapabilities***************/
    
    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getColorDepthCapabilities", params, result);
    }

    /****************setCurrentResolution***************/

    {
        JsonObject params2, result;
        params2["videoDisplay"] = "HDMI0";
        params2["resolution"] = "1080p60";
        params2["persist"] = true;
        params2["ignoreEdid"] = false;

        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "setCurrentResolution", params2, result);
    }


    /****************getCurrentOutputSettings***************/

    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getCurrentOutputSettings", params, result);
    }

    /**************getSupportedResolutions********************/

    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getSupportedResolutions", params, result);
    }

    /****************setForceHDRMode***************/

    {
        JsonObject result, params;
        params["hdr_mode"] = "DV";
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "setForceHDRMode", params, result);
    }

    /**************getCurrentResolution********************/

    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getCurrentResolution", params, result);
    }

   /************************getZoomSetting***************************/
 
    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getZoomSetting", params, result);
    }

     /************************getTVHDRCapabilities***************************/

    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getTVHDRCapabilities", params, result);
    }

    /************************getSinkAtmosCapability***************************/
    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getSinkAtmosCapability", params, result);
    }

   /************************getSupportedMS12Config***************************/
    {
        JsonObject result, params;
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getSupportedMS12Config", params, result);
    }

    /************************getVolumeLeveller***************************/


    {
        JsonObject result, params5;
        params5["audioPort"] = "SPEAKER0";
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getVolumeLeveller", params5, result);
    }

    /************************resetBassEnhancer***************************/

    {
        JsonObject result, params6;
        params6["audioPort"] = "SPEAKER0";
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "resetBassEnhancer", params6, result);
    }

    /************************resetVolumeLeveller***************************/

    {
        JsonObject result, params7;
        params7["audioPort"] = "SPEAKER0";
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "resetVolumeLeveller", params7, result);
    }

    /************************getBassEnhancer***************************/

    {
        JsonObject params3, result;
        params3["audioPort"] = "SPEAKER0";
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getBassEnhancer", params3, result);
    }

    /************************getSurroundVirtualizer***************************/

    {
        JsonObject result, params4;
        params4["audioPort"] = "SPEAKER0";
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "getSurroundVirtualizer", params4, result);
    }

    /************************setVolumeLeveller***************************/

    {
        JsonObject result, params8;
        params8["audioPort"] = "SPEAKER0";
        params8["mode"] = "1";
        params8["level"] = "9";
        status = InvokeServiceMethod("org.rdk.DisplaySettings.1", "setVolumeLeveller", params8, result);
    }

}
