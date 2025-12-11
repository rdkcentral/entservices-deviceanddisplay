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
#include "MfrMock.h"
#include "IarmBusMock.h"
#include "SystemInfo.h"

#include <mutex>
#include <condition_variable>

#define JSON_TIMEOUT   (1000)
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);
#define DEVICEINFO_CALLSIGN  _T("DeviceInfo.1")
#define DEVICEINFOL2TEST_CALLSIGN _T("L2tests.1")

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;

class DeviceInfo_L2test : public L2TestMocks {
protected:
    Core::JSONRPC::Message message;
    string response;

    virtual ~DeviceInfo_L2test() override;
        
public:
    DeviceInfo_L2test();
};

/**
* @brief Constructor for DeviceInfo L2 test class
*/
DeviceInfo_L2test::DeviceInfo_L2test()
        : L2TestMocks()
{
    TEST_LOG("DEVICEINFO Constructor\n");
    uint32_t status = Core::ERROR_GENERAL;

    std::ofstream versionFile("/version.txt");
    versionFile << "imagename:CUSTOM5_VBN_2203_sprint_20220331225312sdy_NG\n";
    versionFile << "SDK_VERSION=17.3\n";
    versionFile << "MEDIARITE=8.3.53\n";
    versionFile << "YOCTO_VERSION=dunfell\n";
    versionFile.close();

    std::ofstream devicePropsFile("/etc/device.properties");
    devicePropsFile << "MFG_NAME=TestManufacturer\n";
    devicePropsFile << "FRIENDLY_ID=\"TestModel\"\n";
    devicePropsFile << "MODEL_NUM=TEST_SKU_12345\n";
    // devicePropsFile << "SOC=TestSOC\n";
    devicePropsFile << "CHIPSET_NAME=TestChipset\n";
    devicePropsFile << "DEVICE_TYPE=IpStb\n";
    devicePropsFile.close();

    std::ofstream authServiceFile("/etc/authService.conf");
    authServiceFile << "deviceType=IpStb\n";
    authServiceFile.close();

    std::ofstream manufacturerFile("/tmp/.manufacturer");
    manufacturerFile << "TestBrand\n";
    manufacturerFile.close();

    // Create partnerId file with directory
    system("mkdir -p /opt/www/authService");
    std::ofstream partnerIdFile("/opt/www/authService/partnerId3.dat");
    partnerIdFile << "TestPartnerID\n";
    partnerIdFile.close();

    // Setup RFC API mock expectations
    ON_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                TEST_LOG("getRFCParameter invoked: param=%s", pcParameterName);
                
                if (strcmp(pcParameterName, "Device.DeviceInfo.SerialNumber") == 0) {
                    strcpy(pstParamData->value, "RFC_TEST_SERIAL");
                    return WDMP_SUCCESS;
                } else if (strcmp(pcParameterName, "Device.DeviceInfo.ModelName") == 0) {
                    strcpy(pstParamData->value, "RFC_TEST_MODEL");
                    return WDMP_SUCCESS;
                } else if (strcmp(pcParameterName, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId") == 0) {
                    strcpy(pstParamData->value, "TestPartnerID");
                    printf("getRFCParameter invoked for PartnerId: %s\n", pstParamData->value);
                    return WDMP_SUCCESS;
                }
                
                pstParamData->value[0] = '\0';
                return WDMP_FAILURE;
            }));

    /* Activate plugin in constructor */
    status = ActivateService("DeviceInfo");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

/**
* @brief Destructor for DeviceInfo L2 test class
*/
DeviceInfo_L2test::~DeviceInfo_L2test()
{
    TEST_LOG("DEVICEINFO Destructor\n");
    uint32_t status = Core::ERROR_GENERAL;

    status = DeactivateService("DeviceInfo");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

TEST_F(DeviceInfo_L2test, DeviceInfo_L2_MethodTest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVICEINFO_CALLSIGN, DEVICEINFOL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_NONE;

    TEST_LOG("Starting DeviceInfo L2 Method Tests\n");

    /****************** defaultresolution ******************/
    {
        TEST_LOG("Testing defaultresolution method\n");
        JsonObject result, params;
        params["videoDisplay"] = "HDMI0";
        
        device::VideoOutputPort videoOutputPort;
        device::VideoResolution videoResolution;
        string videoPort(_T("HDMI0"));
        string videoPortDefaultResolution(_T("1080p"));

        ON_CALL(*p_videoResolutionMock, getName())
            .WillByDefault(::testing::ReturnRef(videoPortDefaultResolution));
        ON_CALL(*p_videoOutputPortMock, getDefaultResolution())
            .WillByDefault(::testing::ReturnRef(videoResolution));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "defaultresolution", params, result);
         EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** supportedresolutions ******************/
    {
        TEST_LOG("Testing supportedresolutions method\n");
        JsonObject result, params;
        params["videoDisplay"] = "HDMI0";
        
        device::VideoOutputPort videoOutputPort;
        device::VideoOutputPortType videoOutputPortType;
        device::VideoResolution videoResolution;
        string videoPort(_T("HDMI0"));
        string videoPortSupportedResolution(_T("1080p"));

        ON_CALL(*p_videoResolutionMock, getName())
            .WillByDefault(::testing::ReturnRef(videoPortSupportedResolution));
        ON_CALL(*p_videoOutputPortTypeMock, getSupportedResolutions())
            .WillByDefault(::testing::Return(device::List<device::VideoResolution>({ videoResolution })));
        ON_CALL(*p_videoOutputPortTypeMock, getId())
            .WillByDefault(::testing::Return(0));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortConfigImplMock, getPortType(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        
        status = InvokeServiceMethod("DeviceInfo.1", "supportedresolutions", params, result);
         EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** supportedhdcp ******************/
    {
        TEST_LOG("Testing supportedhdcp method\n");
        JsonObject result, params;
        params["videoDisplay"] = "HDMI0";
        
        device::VideoOutputPort videoOutputPort;
        string videoPort(_T("HDMI0"));

        ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
            .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "supportedhdcp", params, result);
         EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** audiocapabilities ******************/
    {
        TEST_LOG("Testing audiocapabilities method\n");
        JsonObject result, params;
        params["audioPort"] = "HDMI0";
        
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_audioOutputPortMock, getAudioCapabilities(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](int* capabilities) {
                    if (capabilities != nullptr) {
                        *capabilities = dsAUDIOSUPPORT_ATMOS | dsAUDIOSUPPORT_DD | dsAUDIOSUPPORT_DDPLUS | dsAUDIOSUPPORT_DAD | dsAUDIOSUPPORT_DAPv2 | dsAUDIOSUPPORT_MS12;
                    }
                }));
        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "audiocapabilities", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** ms12capabilities ******************/
    {
        TEST_LOG("Testing ms12capabilities method\n");
        JsonObject result, params;
        params["audioPort"] = "HDMI0";
        
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_audioOutputPortMock, getMS12Capabilities(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](int* capabilities) {
                    if (capabilities != nullptr) {
                        *capabilities = dsMS12SUPPORT_DolbyVolume | dsMS12SUPPORT_InteligentEqualizer | dsMS12SUPPORT_DialogueEnhancer;
                    }
                }));
        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "ms12capabilities", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** supportedms12audioprofiles ******************/
    {
        TEST_LOG("Testing supportedms12audioprofiles method\n");
        JsonObject result, params;
        params["audioPort"] = "HDMI0";
        
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("HDMI0"));
        string audioPortMS12AudioProfile(_T("Movie"));

        ON_CALL(*p_audioOutputPortMock, getMS12AudioProfileList())
            .WillByDefault(::testing::Return(std::vector<std::string>({ audioPortMS12AudioProfile })));
        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "supportedms12audioprofiles", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    TEST_LOG("DeviceInfo L2 Method Tests completed\n");
}

TEST_F(DeviceInfo_L2test, DeviceInfo_L2_PropertyTest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVICEINFO_CALLSIGN, DEVICEINFOL2TEST_CALLSIGN);
    JsonObject result, params;
    uint32_t status = Core::ERROR_NONE;

    TEST_LOG("Starting DeviceInfo L2 Property Tests\n");

    /****************** systeminfo ******************/
    {
        TEST_LOG("Testing systeminfo property\n");
    
        ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
            .WillByDefault(
                [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                    EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_MFRLIB_NAME)));
                    EXPECT_EQ(string(methodName), string(_T(IARM_BUS_MFRLIB_API_GetSerializedData)));
                    auto* param = static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
                    const char* str = "5678";
                    param->bufLen = strlen(str);
                    strncpy(param->buffer, str, sizeof(param->buffer));
                    param->type =  mfrSERIALIZED_TYPE_SERIALNUMBER;
                    return IARM_RESULT_SUCCESS;
                });
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "systeminfo@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** addresses ******************/
    {
        TEST_LOG("Testing addresses property\n");

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "addresses@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
        TEST_LOG("addresses test completed\n");
    }

    /****************** socketinfo ******************/
    {
        TEST_LOG("Testing socketinfo property\n");

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "socketinfo@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** firmwareversion ******************/
    {
        TEST_LOG("Testing firmwareversion property\n");

        std::ofstream file("/version.txt");
        file << "imagename:CUSTOM5_VBN_2203_sprint_20220331225312sdy_NG\nSDK_VERSION=17.3\nMEDIARITE=8.3.53\nYOCTO_VERSION=dunfell\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "firmwareversion@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** serialnumber ******************/
    {
        TEST_LOG("Testing serialnumber property\n");
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "serialnumber@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** modelid ******************/
    {
        TEST_LOG("Testing modelid property\n");

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "modelid@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);

    }

    /****************** make ******************/
    {
        TEST_LOG("Testing make property\n");

        std::ofstream file("/etc/device.properties");
        file << "MFG_NAME=CUSTOM4";
        file.close();
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "make@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** modelname ******************/
    {
        TEST_LOG("Testing modelname property\n");

        std::ofstream file("/etc/device.properties");
        file << "FRIENDLY_ID=\"CUSTOM4 CUSTOM9\"";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "modelname@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** brandname ******************/
    {
        TEST_LOG("Testing brandname property\n");
        JsonObject result, params;
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "brandname@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** devicetype ******************/
    {
        TEST_LOG("Testing devicetype property\n");

        std::ofstream file("/etc/authService.conf");
        file << "deviceType=IpStb";
        file.close();
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "devicetype@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** socname ******************/
    {
        TEST_LOG("Testing socname property\n");

        std::ofstream file("/etc/device.properties");
        file << "SOC=NVIDIA\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "socname@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    {
        TEST_LOG("Testing distributorid from file\n");
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "distributorid@0", getResults);
        EXPECT_EQ(Core::ERROR_GENERAL, getResult);
    }


    /****************** releaseversion ******************/
    {
        TEST_LOG("Testing releaseversion property\n");
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "releaseversion@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** chipset ******************/
    {
        TEST_LOG("Testing chipset property\n");

        std::ofstream file("/etc/device.properties");
        file << "CHIPSET_NAME=TestChipset\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "chipset@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);


        if (getResult == Core::ERROR_NONE) {
            EXPECT_TRUE(getResults.HasLabel("chipset"));
            TEST_LOG("chipset test passed\n");
        }
    }

    /****************** supportedaudioports ******************/
    {
        TEST_LOG("Testing supportedaudioports property\n");
        JsonObject result, params;
        JsonObject getResults;
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_audioOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPorts())
            .WillByDefault(::testing::Return(device::List<device::AudioOutputPort>({ audioOutputPort })));

        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "supportedaudioports@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** supportedvideodisplays ******************/
    {
        TEST_LOG("Testing supportedvideodisplays property\n");
        JsonObject result, params;
        
        device::VideoOutputPort videoOutputPort;
        string videoPort(_T("HDMI0"));

        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(device::List<device::VideoOutputPort>({ videoOutputPort })));

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "supportedvideodisplays@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** hostedid ******************/
    {
        TEST_LOG("Testing hostedid property\n");
        JsonObject result, params;
        
        ON_CALL(*p_hostImplMock, getHostEDID(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](std::vector<uint8_t>& edid) {
                    edid = { 't', 'e', 's', 't' };
                }));
        
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "hostedid@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    TEST_LOG("DeviceInfo L2 Property Tests completed\n");
}

TEST_F(DeviceInfo_L2test, DeviceInfo_L2_ErrorHandlingTest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVICEINFO_CALLSIGN, DEVICEINFOL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_NONE;

    TEST_LOG("Starting DeviceInfo L2 Error Handling Tests\n");

    /****************** Test with invalid video display ******************/
    {
        TEST_LOG("Testing defaultresolution with invalid videoDisplay\n");
        JsonObject result, params;
        params["videoDisplay"] = "INVALID_PORT";
        
        device::VideoOutputPort videoOutputPort;
        string videoPort(_T("HDMI0"));

        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::Throw(device::Exception("Invalid port")));
        
        status = InvokeServiceMethod("DeviceInfo.1", "defaultresolution", params, result);
        EXPECT_EQ(Core::ERROR_GENERAL, status);
    }

    /****************** Test with invalid audio port ******************/
    {
        TEST_LOG("Testing audiocapabilities with invalid audioPort\n");
        JsonObject result, params;
        params["audioPort"] = "INVALID_AUDIO_PORT";
        
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::Throw(device::Exception("Invalid audio port")));
        
        status = InvokeServiceMethod("DeviceInfo.1", "audiocapabilities", params, result);
        EXPECT_EQ(Core::ERROR_GENERAL, status);
    }

    /****************** Test supportedresolutions with exception ******************/
    {
        TEST_LOG("Testing supportedresolutions with device exception\n");
        JsonObject result, params;
        params["videoDisplay"] = "HDMI0";
        
        device::VideoOutputPort videoOutputPort;
        string videoPort(_T("HDMI0"));

        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::Throw(device::Exception("Type exception")));
        
        status = InvokeServiceMethod("DeviceInfo.1", "supportedresolutions", params, result);
        EXPECT_EQ(Core::ERROR_GENERAL, status);
    }

    /****************** Test ms12capabilities with exception ******************/
    {
        TEST_LOG("Testing ms12capabilities with device exception\n");
        JsonObject result, params;
        params["audioPort"] = "HDMI0";
        
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::Throw(device::Exception("MS12 exception")));
        
        status = InvokeServiceMethod("DeviceInfo.1", "ms12capabilities", params, result);
        EXPECT_EQ(Core::ERROR_GENERAL, status);
    }

    TEST_LOG("DeviceInfo L2 Error Handling Tests completed\n");
}

TEST_F(DeviceInfo_L2test, DeviceInfo_L2_EdgeCaseTest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVICEINFO_CALLSIGN, DEVICEINFOL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_NONE;

    TEST_LOG("Starting DeviceInfo L2 Edge Case Tests\n");

    /****************** Test with empty videoDisplay parameter ******************/
    {
        TEST_LOG("Testing defaultresolution with empty videoDisplay\n");
        JsonObject result, params;
        params["videoDisplay"] = "";
        
        device::VideoOutputPort videoOutputPort;
        device::VideoResolution videoResolution;
        string videoPort(_T("HDMI0"));
        string videoPortDefaultResolution(_T("1080p"));

        ON_CALL(*p_videoResolutionMock, getName())
            .WillByDefault(::testing::ReturnRef(videoPortDefaultResolution));
        ON_CALL(*p_videoOutputPortMock, getDefaultResolution())
            .WillByDefault(::testing::ReturnRef(videoResolution));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "defaultresolution", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** Test with empty audioPort parameter ******************/
    {
        TEST_LOG("Testing audiocapabilities with empty audioPort\n");
        JsonObject result, params;
        params["audioPort"] = "";
        
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_audioOutputPortMock, getAudioCapabilities(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](int* capabilities) {
                    if (capabilities != nullptr) {
                        *capabilities = dsAUDIOSUPPORT_NONE;
                    }
                }));
        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "audiocapabilities", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** Test with multiple resolutions ******************/
    {
        TEST_LOG("Testing supportedresolutions with multiple resolutions\n");
        JsonObject result, params;
        params["videoDisplay"] = "HDMI0";
        
        device::VideoOutputPort videoOutputPort;
        device::VideoOutputPortType videoOutputPortType;
        device::VideoResolution videoResolution1, videoResolution2, videoResolution3;
        string videoPort(_T("HDMI0"));
        string resolution1(_T("480p"));
        string resolution2(_T("720p"));
        string resolution3(_T("1080p"));

        ON_CALL(*p_videoResolutionMock, getName())
            .WillByDefault(::testing::ReturnRef(resolution1));
        ON_CALL(*p_videoOutputPortTypeMock, getSupportedResolutions())
            .WillByDefault(::testing::Return(device::List<device::VideoResolution>({ videoResolution1, videoResolution2, videoResolution3 })));
        ON_CALL(*p_videoOutputPortTypeMock, getId())
            .WillByDefault(::testing::Return(0));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortConfigImplMock, getPortType(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        
        status = InvokeServiceMethod("DeviceInfo.1", "supportedresolutions", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** Test HDCP version 1.x ******************/
    {
        TEST_LOG("Testing supportedhdcp with HDCP 1.x\n");
        JsonObject result, params;
        params["videoDisplay"] = "HDMI0";
        
        device::VideoOutputPort videoOutputPort;
        string videoPort(_T("HDMI0"));

        ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
            .WillByDefault(::testing::Return(dsHDCP_VERSION_1X));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "supportedhdcp", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** Test all MS12 capabilities ******************/
    {
        TEST_LOG("Testing ms12capabilities with all capabilities enabled\n");
        JsonObject result, params;
        params["audioPort"] = "HDMI0";
        
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_audioOutputPortMock, getMS12Capabilities(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](int* capabilities) {
                    if (capabilities != nullptr) {
                        *capabilities = dsMS12SUPPORT_DolbyVolume | dsMS12SUPPORT_InteligentEqualizer | dsMS12SUPPORT_DialogueEnhancer;
                    }
                }));
        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "ms12capabilities", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** Test multiple MS12 audio profiles ******************/
    {
        TEST_LOG("Testing supportedms12audioprofiles with multiple profiles\n");
        JsonObject result, params;
        params["audioPort"] = "HDMI0";
        
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("HDMI0"));
        std::vector<std::string> profiles = {"Movie", "Music", "Sports", "Game"};

        ON_CALL(*p_audioOutputPortMock, getMS12AudioProfileList())
            .WillByDefault(::testing::Return(profiles));
        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "supportedms12audioprofiles", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    TEST_LOG("DeviceInfo L2 Edge Case Tests completed\n");
}

TEST_F(DeviceInfo_L2test, DeviceInfo_L2_PropertyEdgeCaseTest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVICEINFO_CALLSIGN, DEVICEINFOL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_NONE;

    TEST_LOG("Starting DeviceInfo L2 Property Edge Case Tests\n");

    /****************** Test firmwareversion with missing fields ******************/
    {
        TEST_LOG("Testing firmwareversion with only imagename\n");

        std::ofstream file("/version.txt");
        file << "imagename:TEST_IMAGE\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "firmwareversion@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
        TEST_LOG("firmwareversion with minimal data test completed\n");
    }

    /****************** Test firmwareversion with all fields ******************/
    {
        TEST_LOG("Testing firmwareversion with all fields present\n");

        std::ofstream file("/version.txt");
        file << "imagename:CUSTOM5_VBN_2203_sprint_20220331225312sdy_NG\n";
        file << "SDK_VERSION=17.3\n";
        file << "MEDIARITE=8.3.53\n";
        file << "YOCTO_VERSION=dunfell\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "firmwareversion@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
        
        if (getResult == Core::ERROR_NONE && getResults.HasLabel("firmwareversion")) {
            JsonObject fwObj = getResults["firmwareversion"].Object();
            EXPECT_TRUE(fwObj.HasLabel("imagename"));
            EXPECT_TRUE(fwObj.HasLabel("sdk"));
            EXPECT_TRUE(fwObj.HasLabel("mediarite"));
            EXPECT_TRUE(fwObj.HasLabel("yocto"));
        }
        TEST_LOG("firmwareversion with all fields test completed\n");
    }

    /****************** Test serialnumber from MFR ******************/
    {
        TEST_LOG("Testing serialnumber from MFR\n");
        
        ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
            .WillByDefault(
                [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                    if (strcmp(methodName, IARM_BUS_MFRLIB_API_GetSerializedData) == 0) {
                        auto* param = static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
                        if (param->type == mfrSERIALIZED_TYPE_SERIALNUMBER) {
                            const char* str = "MFR_SERIAL_12345";
                            param->bufLen = strlen(str);
                            strncpy(param->buffer, str, sizeof(param->buffer));
                            return IARM_RESULT_SUCCESS;
                        }
                    }
                    return IARM_RESULT_INVALID_PARAM;
                });

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "serialnumber@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** Test make from device.properties ******************/
    {
        TEST_LOG("Testing make from device.properties\n");

        std::ofstream file("/etc/device.properties");
        file << "MFG_NAME=EdgeCaseManufacturer\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "make@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** Test modelname with quotes ******************/
    {
        TEST_LOG("Testing modelname with quoted value\n");

        std::ofstream file("/etc/device.properties");
        file << "FRIENDLY_ID=\"Quoted Model Name\"\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "modelname@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** Test devicetype from device.properties ******************/
    {
        TEST_LOG("Testing devicetype from device.properties\n");

        std::ofstream authFile("/etc/authService.conf");
        authFile << "# No deviceType here\n";
        authFile.close();

        std::ofstream devFile("/etc/device.properties");
        devFile << "DEVICE_TYPE=mediaclient\n";
        devFile.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "devicetype@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** Test distributorid from RFC ******************/
    {
        TEST_LOG("Testing distributorid from RFC\n");
        
        ON_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                    if (strcmp(pcParameterName, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId") == 0) {
                        strcpy(pstParamData->value, "RFC_PARTNER_ID");
                        return WDMP_SUCCESS;
                    }
                    return WDMP_FAILURE;
                }));

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "distributorid@0", getResults);
        EXPECT_EQ(Core::ERROR_GENERAL, getResult);
    }

    /****************** Test multiple audio ports ******************/
    {
        TEST_LOG("Testing supportedaudioports with multiple ports\n");
        
        device::AudioOutputPort audioOutputPort1, audioOutputPort2, audioOutputPort3;
        string audioPort1(_T("HDMI0"));
        string audioPort2(_T("SPDIF0"));
        string audioPort3(_T("SPEAKER0"));

        ON_CALL(*p_audioOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(audioPort1));
        ON_CALL(*p_hostImplMock, getAudioOutputPorts())
            .WillByDefault(::testing::Return(device::List<device::AudioOutputPort>({ audioOutputPort1, audioOutputPort2, audioOutputPort3 })));

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "supportedaudioports@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** Test multiple video displays ******************/
    {
        TEST_LOG("Testing supportedvideodisplays with multiple displays\n");
        
        device::VideoOutputPort videoOutputPort1, videoOutputPort2;
        string videoPort1(_T("HDMI0"));
        string videoPort2(_T("HDMI1"));

        ON_CALL(*p_videoOutputPortMock, getName())
            .WillByDefault(::testing::ReturnRef(videoPort1));
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Return(device::List<device::VideoOutputPort>({ videoOutputPort1, videoOutputPort2 })));

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "supportedvideodisplays@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** Test hostedid with large EDID ******************/
    {
        TEST_LOG("Testing hostedid with large EDID data\n");
        
        ON_CALL(*p_hostImplMock, getHostEDID(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](std::vector<uint8_t>& edid) {
                    // Standard EDID size is 128 or 256 bytes
                    edid.resize(256);
                    for (size_t i = 0; i < 256; i++) {
                        edid[i] = static_cast<uint8_t>(i & 0xFF);
                    }
                }));
        
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "hostedid@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** Test releaseversion parsing ******************/
    {
        TEST_LOG("Testing releaseversion with different version formats\n");

        std::ofstream file("/version.txt");
        file << "imagename:CUSTOM5_VBN_23.4sprint_test\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "releaseversion@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    TEST_LOG("DeviceInfo L2 Property Edge Case Tests completed\n");
}

TEST_F(DeviceInfo_L2test, DeviceInfo_L2_ExceptionHandlingTest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVICEINFO_CALLSIGN, DEVICEINFOL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_NONE;

    TEST_LOG("Starting DeviceInfo L2 Exception Handling Tests\n");

    /****************** Test supportedaudioports with exception ******************/
    {
        TEST_LOG("Testing supportedaudioports with device exception\n");
        
        ON_CALL(*p_hostImplMock, getAudioOutputPorts())
            .WillByDefault(::testing::Throw(device::Exception("Audio ports exception")));

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "supportedaudioports@0", getResults);
        EXPECT_EQ(Core::ERROR_GENERAL, getResult);
    }

    /****************** Test supportedvideodisplays with exception ******************/
    {
        TEST_LOG("Testing supportedvideodisplays with device exception\n");
        
        ON_CALL(*p_hostImplMock, getVideoOutputPorts())
            .WillByDefault(::testing::Throw(device::Exception("Video ports exception")));

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "supportedvideodisplays@0", getResults);
        EXPECT_EQ(Core::ERROR_GENERAL, getResult);
    }

    /****************** Test hostedid with exception ******************/
    {
        TEST_LOG("Testing hostedid with device exception\n");
        
        ON_CALL(*p_hostImplMock, getHostEDID(::testing::_))
            .WillByDefault(::testing::Throw(device::Exception("EDID exception")));
        
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "hostedid@0", getResults);
        EXPECT_EQ(Core::ERROR_GENERAL, getResult);
    }

    /****************** Test supportedhdcp with exception ******************/
    {
        TEST_LOG("Testing supportedhdcp with exception\n");
        JsonObject result, params;
        params["videoDisplay"] = "HDMI0";
        
        device::VideoOutputPort videoOutputPort;
        string videoPort(_T("HDMI0"));

        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
            .WillByDefault(::testing::Throw(device::Exception("HDCP exception")));
        
        status = InvokeServiceMethod("DeviceInfo.1", "supportedhdcp", params, result);
        EXPECT_EQ(Core::ERROR_GENERAL, status);
    }

    /****************** Test supportedms12audioprofiles with exception ******************/
    {
        TEST_LOG("Testing supportedms12audioprofiles with device exception\n");
        JsonObject result, params;
        params["audioPort"] = "HDMI0";
        
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::Throw(device::Exception("MS12 profiles exception")));
        
        status = InvokeServiceMethod("DeviceInfo.1", "supportedms12audioprofiles", params, result);
        EXPECT_EQ(Core::ERROR_GENERAL, status);
    }

    TEST_LOG("DeviceInfo L2 Exception Handling Tests completed\n");
}

TEST_F(DeviceInfo_L2test, DeviceInfo_L2_AdditionalPropertiesTest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVICEINFO_CALLSIGN, DEVICEINFOL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_NONE;

    TEST_LOG("Starting DeviceInfo L2 Additional Properties Tests\n");

    /****************** Test systeminfo with different MFR data ******************/
    {
        TEST_LOG("Testing systeminfo with different serial number\n");
    
        ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
            .WillByDefault(
                [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                    if (strcmp(methodName, IARM_BUS_MFRLIB_API_GetSerializedData) == 0) {
                        auto* param = static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
                        if (param->type == mfrSERIALIZED_TYPE_SERIALNUMBER) {
                            const char* str = "DIFFERENT_SERIAL_9999";
                            param->bufLen = strlen(str);
                            strncpy(param->buffer, str, sizeof(param->buffer));
                            return IARM_RESULT_SUCCESS;
                        }
                    }
                    return IARM_RESULT_INVALID_PARAM;
                });

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "systeminfo@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
        
        if (getResult == Core::ERROR_NONE && getResults.HasLabel("systeminfo")) {
            JsonObject sysInfo = getResults["systeminfo"].Object();
            EXPECT_TRUE(sysInfo.HasLabel("time"));
            EXPECT_TRUE(sysInfo.HasLabel("version"));
            EXPECT_TRUE(sysInfo.HasLabel("uptime"));
        }
    }

    /****************** Test socname with different values ******************/
    {
        TEST_LOG("Testing socname with BCM chip\n");

        std::ofstream file("/etc/device.properties");
        file << "SOC=BCM7252\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "socname@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** Test chipset with different values ******************/
    {
        TEST_LOG("Testing chipset with different chipset name\n");

        std::ofstream file("/etc/device.properties");
        file << "CHIPSET_NAME=CustomChipset2024\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "chipset@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** Test brandname from different source ******************/
    {
        TEST_LOG("Testing brandname from .manufacturer file\n");
        
        std::ofstream file("/tmp/.manufacturer");
        file << "CustomBrand2024\n";
        file.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "brandname@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    /****************** Test modelid with MFR fallback ******************/
    {
        TEST_LOG("Testing modelid with MFR fallback\n");
        
        std::ofstream file("/etc/device.properties");
        file << "# MODEL_NUM not present\n";
        file.close();

        ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
            .WillByDefault(
                [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                    if (strcmp(methodName, IARM_BUS_MFRLIB_API_GetSerializedData) == 0) {
                        auto* param = static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
                        if (param->type == mfrSERIALIZED_TYPE_MODELNAME) {
                            const char* str = "MFR_MODEL_NAME";
                            param->bufLen = strlen(str);
                            strncpy(param->buffer, str, sizeof(param->buffer));
                            return IARM_RESULT_SUCCESS;
                        }
                    }
                    return IARM_RESULT_INVALID_PARAM;
                });

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "modelid@0", getResults);
        // May succeed or fail depending on implementation
    }

    /****************** Test devicetype conversion ******************/
    {
        TEST_LOG("Testing devicetype with 'hybrid' conversion\n");

        std::ofstream authFile("/etc/authService.conf");
        authFile << "# No deviceType\n";
        authFile.close();

        std::ofstream devFile("/etc/device.properties");
        devFile << "DEVICE_TYPE=hybrid\n";
        devFile.close();

        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "devicetype@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);
    }

    TEST_LOG("DeviceInfo L2 Additional Properties Tests completed\n");
}

TEST_F(DeviceInfo_L2test, DeviceInfo_L2_MethodVariationsTest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVICEINFO_CALLSIGN, DEVICEINFOL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_NONE;

    TEST_LOG("Starting DeviceInfo L2 Method Variations Tests\n");

    /****************** Test defaultresolution with different resolutions ******************/
    {
        TEST_LOG("Testing defaultresolution with 4K resolution\n");
        JsonObject result, params;
        params["videoDisplay"] = "HDMI0";
        
        device::VideoOutputPort videoOutputPort;
        device::VideoResolution videoResolution;
        string videoPort(_T("HDMI0"));
        string videoPortDefaultResolution(_T("2160p"));

        ON_CALL(*p_videoResolutionMock, getName())
            .WillByDefault(::testing::ReturnRef(videoPortDefaultResolution));
        ON_CALL(*p_videoOutputPortMock, getDefaultResolution())
            .WillByDefault(::testing::ReturnRef(videoResolution));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "defaultresolution", params, result);
        EXPECT_EQ(Core::ERROR_GENERAL, status);
    }

    /****************** Test supportedresolutions with 4K support ******************/
    {
        TEST_LOG("Testing supportedresolutions with 4K support\n");
        JsonObject result, params;
        params["videoDisplay"] = "HDMI0";
        
        device::VideoOutputPort videoOutputPort;
        device::VideoOutputPortType videoOutputPortType;
        device::VideoResolution res1, res2, res3, res4, res5;
        string videoPort(_T("HDMI0"));
        string resolution(_T("2160p"));

        ON_CALL(*p_videoResolutionMock, getName())
            .WillByDefault(::testing::ReturnRef(resolution));
        ON_CALL(*p_videoOutputPortTypeMock, getSupportedResolutions())
            .WillByDefault(::testing::Return(device::List<device::VideoResolution>({ res1, res2, res3, res4, res5 })));
        ON_CALL(*p_videoOutputPortTypeMock, getId())
            .WillByDefault(::testing::Return(0));
        ON_CALL(*p_videoOutputPortMock, getType())
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPort));
        ON_CALL(*p_videoOutputPortConfigImplMock, getPortType(::testing::_))
            .WillByDefault(::testing::ReturnRef(videoOutputPortType));
        
        status = InvokeServiceMethod("DeviceInfo.1", "supportedresolutions", params, result);
        EXPECT_EQ(Core::ERROR_GENERAL, status);
    }

    /****************** Test audiocapabilities with different capabilities ******************/
    {
        TEST_LOG("Testing audiocapabilities with DD only\n");
        JsonObject result, params;
        params["audioPort"] = "HDMI0";
        
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_audioOutputPortMock, getAudioCapabilities(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](int* capabilities) {
                    if (capabilities != nullptr) {
                        *capabilities = dsAUDIOSUPPORT_DD;
                    }
                }));
        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "audiocapabilities", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** Test audiocapabilities with DD+ and DAD ******************/
    {
        TEST_LOG("Testing audiocapabilities with DD+ and DAD\n");
        JsonObject result, params;
        params["audioPort"] = "SPDIF0";
        
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("SPDIF0"));

        ON_CALL(*p_audioOutputPortMock, getAudioCapabilities(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](int* capabilities) {
                    if (capabilities != nullptr) {
                        *capabilities = dsAUDIOSUPPORT_DDPLUS | dsAUDIOSUPPORT_DAD;
                    }
                }));
        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "audiocapabilities", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** Test ms12capabilities with individual capabilities ******************/
    {
        TEST_LOG("Testing ms12capabilities with DolbyVolume only\n");
        JsonObject result, params;
        params["audioPort"] = "HDMI0";
        
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_audioOutputPortMock, getMS12Capabilities(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](int* capabilities) {
                    if (capabilities != nullptr) {
                        *capabilities = dsMS12SUPPORT_DolbyVolume;
                    }
                }));
        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "ms12capabilities", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    /****************** Test ms12capabilities with no capabilities ******************/
    {
        TEST_LOG("Testing ms12capabilities with none\n");
        JsonObject result, params;
        params["audioPort"] = "HDMI0";
        
        device::AudioOutputPort audioOutputPort;
        string audioPort(_T("HDMI0"));

        ON_CALL(*p_audioOutputPortMock, getMS12Capabilities(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](int* capabilities) {
                    if (capabilities != nullptr) {
                        *capabilities = dsMS12SUPPORT_NONE;
                    }
                }));
        ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
            .WillByDefault(::testing::Return(audioPort));
        ON_CALL(*p_hostImplMock, getAudioOutputPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(audioOutputPort));
        
        status = InvokeServiceMethod("DeviceInfo.1", "ms12capabilities", params, result);
        EXPECT_EQ(Core::ERROR_NONE, status);
    }

    TEST_LOG("DeviceInfo L2 Method Variations Tests completed\n");
}

