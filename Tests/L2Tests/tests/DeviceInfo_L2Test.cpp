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


//    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
//         .WillByDefault(
//             [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
//                 EXPECT_EQ(string(ownerName), string(_T(IARM_BUS_MFRLIB_NAME)));
//                 EXPECT_EQ(string(methodName), string(_T(IARM_BUS_MFRLIB_API_GetSerializedData)));
//                 auto* param = static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
//                 const char* str = "5678";
//                 param->bufLen = strlen(str);
//                 strncpy(param->buffer, str, sizeof(param->buffer));
//                 param->type =  mfrSERIALIZED_TYPE_SERIALNUMBER;
//                 return IARM_RESULT_SUCCESS;
//             });


    // ON_CALL(*p_iarmBusImplMock, IARM_Bus_Init(::testing::_))
    //     .WillByDefault(::testing::Return(IARM_RESULT_SUCCESS));

    // ON_CALL(*p_iarmBusImplMock, IARM_Bus_Connect())
    //     .WillByDefault(::testing::Return(IARM_RESULT_SUCCESS));

    // ON_CALL(*p_iarmBusImplMock, IARM_Bus_IsConnected(::testing::_, ::testing::_))
    //     .WillByDefault(::testing::Invoke(
    //         [](const char* memberName, int* isRegistered) {
    //             *isRegistered = 1;  // Return connected
    //             return IARM_RESULT_SUCCESS;
    //         }));

    // Setup IARM Bus expectations
    // ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(::testing::_, ::testing::_, ::testing::_, ::testing::_))
    //     .WillByDefault(::testing::Invoke(
    //         [&](const char* ownerName, const char* methodName, void* arg, size_t argLen) -> IARM_Result_t {
    //             TEST_LOG("IARM_Bus_Call invoked: owner=%s, method=%s", ownerName, methodName);
                
    //             if (strcmp(methodName, IARM_BUS_MFRLIB_API_GetSerializedData) == 0) {
    //                 IARM_Bus_MFRLib_GetSerializedData_Param_t* param = 
    //                     static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
                    
    //                 switch(param->type) {
    //                     case mfrSERIALIZED_TYPE_SERIALNUMBER:
    //                         strcpy(param->buffer, "TEST123456789");
    //                         param->bufLen = strlen(param->buffer);
    //                         break;
    //                     case mfrSERIALIZED_TYPE_MODELNAME:
    //                         strcpy(param->buffer, "TestModel");
    //                         param->bufLen = strlen(param->buffer);
    //                         break;
    //                     case mfrSERIALIZED_TYPE_MANUFACTURER:
    //                         strcpy(param->buffer, "TestManufacturer");
    //                         param->bufLen = strlen(param->buffer);
    //                         break;
    //                     case mfrSERIALIZED_TYPE_PROVISIONED_MODELNAME:
    //                         strcpy(param->buffer, "TestProvisionedModel");
    //                         param->bufLen = strlen(param->buffer);
    //                         break;
    //                     default:
    //                         param->bufLen = 0;
    //                         return IARM_RESULT_INVALID_PARAM;
    //                 }
    //                 return IARM_RESULT_SUCCESS;
    //             }
                
    //             return IARM_RESULT_SUCCESS;
    //         }));

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


    // ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
    //     .WillByDefault(::testing::Return(std::string("HDMI0")));
    
    // ON_CALL(*p_hostImplMock,    // ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
    //     .WillByDefault(::testing::Return(std::string("HDMI0")));
    
    // ON_CALL(*p_hostImplMock, getDefaultAudioPortName())
    //     .WillByDefault(::testing::Return(std::string("HDMI0"))); getDefaultAudioPortName())
    //     .WillByDefault(::testing::Return(std::string("HDMI0")));


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
        // if (status == Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("defaultResolution"));
        //     string resolution = result["defaultResolution"].String();
        //     EXPECT_EQ(resolution, "1080p");
        //     TEST_LOG("defaultresolution: %s\n", resolution.c_str());
        // } else {
        //     TEST_LOG("defaultresolution test skipped\n");
        // }
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
        // if (status == Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("supportedResolutions"));
        //     TEST_LOG("supportedresolutions test passed\n");
        // } else {
        //     TEST_LOG("supportedresolutions test skipped\n");
        // }
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
        //  EXPECT_EQ(hdcpVersion, "2.2");
        // if (status == Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("supportedHDCPVersion"));
        //     string hdcpVersion = result["supportedHDCPVersion"].String();
        //     EXPECT_EQ(hdcpVersion, "2.2");
        //     TEST_LOG("supportedhdcp: %s\n", hdcpVersion.c_str());
        // } else {
        //     TEST_LOG("supportedhdcp test skipped\n");
        // }
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
        // if (status == Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("AudioCapabilities"));
        //     TEST_LOG("audiocapabilities test passed\n");
        // } else {
        //     TEST_LOG("audiocapabilities test skipped\n");
        // }
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
        // if (status == Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("MS12Capabilities"));
        //     TEST_LOG("ms12capabilities test passed\n");
        // } else {
        //     TEST_LOG("ms12capabilities test skipped\n");
        // }
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
        // if (status == Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("supportedMS12AudioProfiles"));
        //     TEST_LOG("supportedms12audioprofiles test passed\n");
        // } else {
        //     TEST_LOG("supportedms12audioprofiles test skipped\n");
        // }
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

        // JsonObject result, params;
        // status = InvokeServiceMethod("DeviceInfo.1", "systeminfo@", params, result);
        // EXPECT_EQ(Core::ERROR_NONE, status);
        // if (status == Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("version"));
        //     EXPECT_TRUE(result.HasLabel("uptime"));
        //     EXPECT_TRUE(result.HasLabel("totalram"));
        //     EXPECT_TRUE(result.HasLabel("freeram"));
        //     EXPECT_TRUE(result.HasLabel("totalswap"));
        //     EXPECT_TRUE(result.HasLabel("freeswap"));
        //     EXPECT_TRUE(result.HasLabel("devicename"));
        //     EXPECT_TRUE(result.HasLabel("cpuload"));
        //     EXPECT_TRUE(result.HasLabel("serialnumber"));
        //     TEST_LOG("systeminfo test passed\n");
        // }
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

    /****************** distributorid ******************/
    // {
    //     TEST_LOG("Testing distributorid property\n");

    //     system("mkdir -p /opt/www/authService");
    //     std::ofstream partnerIdFile("/opt/www/authService/partnerId3.dat");
    //     partnerIdFile << "TestPartnerID\n";
    //     partnerIdFile.close();

    //     // NOW expect RFC to be called exactly once
    //     EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::_, ::testing::_))
    //         .Times(1)
    //         .WillOnce(::testing::Invoke(
    //             [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
    //                 EXPECT_EQ(string(pcParameterName), 
    //                         string("Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId"));
    //                 strcpy(pstParamData->value, "TestPartnerID");
    //                 return WDMP_SUCCESS;
    //             }));

    //     JsonObject getResults;
    //     uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "distributorid@0", getResults);
    //     EXPECT_EQ(Core::ERROR_GENERAL, getResult);

    //     if (getResult == Core::ERROR_NONE) {
    //     EXPECT_TRUE(getResults.HasLabel("distributorid"));
    //     string distributorId = getResults["distributorid"].String();
    //     EXPECT_EQ(distributorId, "comcast");
    //     TEST_LOG("distributorid from RFC: %s\n", distributorId.c_str());
    //     }

    // }

{
    TEST_LOG("Testing distributorid from file\n");

    // system("mkdir -p /opt/www/authService");
    // std::ofstream partnerIdFile("/opt/www/authService/partnerId3.dat");
    // partnerIdFile << "distributorId=comcast\n";
    // partnerIdFile.close();

    JsonObject getResults;
    uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "distributorid@0", getResults);
    EXPECT_EQ(Core::ERROR_GENERAL, getResult);

    // if (getResult == Core::ERROR_NONE) {
    //     EXPECT_TRUE(getResults.HasLabel("distributorid"));
    //     string distributorId = getResults["distributorid"].String();
    //     EXPECT_EQ(distributorId, "comcast");
    //     TEST_LOG("distributorid from file: %s\n", distributorId.c_str());
    // }
}

/****************** distributorid - RFC fallback ******************/
// {
//     TEST_LOG("Testing distributorid RFC fallback\n");

//     // Remove file to force RFC fallback
//     system("rm -f /opt/www/authService/partnerId3.dat");

//     // NOW expect RFC to be called exactly once
//     // EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::_, ::testing::_))
//     //     .Times(1)
//     //     .WillOnce(::testing::Invoke(
//     //         [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
//     //             EXPECT_EQ(string(pcParameterName), 
//     //                      string("Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId"));
//     //             strcpy(pstParamData->value, "TestPartnerID");
//     //             return WDMP_SUCCESS;
//     //         }));

//     JsonObject getResults;
//     uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "distributorid@0", getResults);
//     EXPECT_EQ(Core::ERROR_NONE, getResult);
//     // EXPECT_EQ(distributorId, "TestPartnerID");

//     // if (getResult == Core::ERROR_NONE) {
//     //     EXPECT_TRUE(getResults.HasLabel("distributorid"));
//     //     string distributorId = getResults["distributorid"].String();
//     //     EXPECT_EQ(distributorId, "TestPartnerID");  // From RFC
//     //     TEST_LOG("distributorid from RFC: %s\n", distributorId.c_str());
//     // }
// }

    /****************** releaseversion ******************/
    {
        TEST_LOG("Testing releaseversion property\n");
        JsonObject getResults;
        uint32_t getResult = InvokeServiceMethod(DEVICEINFO_CALLSIGN, "releaseversion@0", getResults);
        EXPECT_EQ(Core::ERROR_NONE, getResult);

        // JsonObject result, params;
        // status = InvokeServiceMethod("DeviceInfo.1", "releaseversion", params, result);
        //  EXPECT_EQ(Core::ERROR_NONE, status);
        // if (getResult== Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("releaseversion"));
        //     TEST_LOG("releaseversion test passed\n");
        // }
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

        // status = InvokeServiceMethod("DeviceInfo.1", "supportedaudioports", params, result);
        //  EXPECT_EQ(Core::ERROR_NONE, status);
        // if (getResult== Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("supportedAudioPorts"));
        //     TEST_LOG("supportedaudioports test passed\n");
        // }
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

        // status = InvokeServiceMethod("DeviceInfo.1", "supportedvideodisplays", params, result);
        //  EXPECT_EQ(Core::ERROR_NONE, status);
        // if (getResult== Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("supportedVideoDisplays"));
        //     TEST_LOG("supportedvideodisplays test passed\n");
        // }
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
        // if (getResult== Core::ERROR_NONE) {
        //     EXPECT_TRUE(result.HasLabel("EDID"));
        //     TEST_LOG("hostedid test passed\n");
        // }
    }

    TEST_LOG("DeviceInfo L2 Property Tests completed\n");
}

