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

#include "DeviceInfo.h"
#include "DeviceInfoImplementation.h"

#include "AudioOutputPortMock.h"
#include "HostMock.h"
#include "IarmBusMock.h"
#include "ManagerMock.h"
#include "ServiceMock.h"
#include "VideoOutputPortConfigMock.h"
#include "VideoOutputPortMock.h"
#include "VideoOutputPortTypeMock.h"
#include "VideoResolutionMock.h"
#include "RfcApiMock.h"
//#include "ISubSystemMock.h"

#include "SystemInfo.h"

#include <fstream>
#include "ThunderPortability.h"

using namespace WPEFramework;

using ::testing::NiceMock;
using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::ReturnRef;

namespace {
const string webPrefix = _T("/Service/DeviceInfo");
}

class DeviceInfoTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::DeviceInfo> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    string response;

    IarmBusImplMock* p_iarmBusImplMock = nullptr;
    ManagerImplMock* p_managerImplMock = nullptr;
    HostImplMock* p_hostImplMock = nullptr;
    AudioOutputPortMock* p_audioOutputPortMock = nullptr;
    VideoResolutionMock* p_videoResolutionMock = nullptr;
    VideoOutputPortMock* p_videoOutputPortMock = nullptr;
    VideoOutputPortConfigImplMock* p_videoOutputPortConfigImplMock = nullptr;
    VideoOutputPortTypeMock* p_videoOutputPortTypeMock = nullptr;
    RfcApiImplMock* p_rfcApiImplMock = nullptr;
    NiceMock<ServiceMock> service;
   // Core::Sink<NiceMock<SystemInfo>> subSystem;

    DeviceInfoTest()
        : plugin(Core::ProxyType<Plugin::DeviceInfo>::Create())
        , handler(*plugin)
        , INIT_CONX(1, 0)
    {
        p_iarmBusImplMock = new NiceMock<IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        p_managerImplMock = new NiceMock<ManagerImplMock>;
        device::Manager::setImpl(p_managerImplMock);

        p_hostImplMock = new NiceMock<HostImplMock>;
        device::Host::setImpl(p_hostImplMock);

        p_audioOutputPortMock = new NiceMock<AudioOutputPortMock>;
        device::AudioOutputPort::setImpl(p_audioOutputPortMock);

        p_videoResolutionMock = new NiceMock<VideoResolutionMock>;
        device::VideoResolution::setImpl(p_videoResolutionMock);

        p_videoOutputPortMock = new NiceMock<VideoOutputPortMock>;
        device::VideoOutputPort::setImpl(p_videoOutputPortMock);

        p_videoOutputPortConfigImplMock = new NiceMock<VideoOutputPortConfigImplMock>;
        device::VideoOutputPortConfig::setImpl(p_videoOutputPortConfigImplMock);

        p_videoOutputPortTypeMock = new NiceMock<VideoOutputPortTypeMock>;
        device::VideoOutputPortType::setImpl(p_videoOutputPortTypeMock);

        p_rfcApiImplMock = new NiceMock<RfcApiImplMock>;
        RfcApi::setImpl(p_rfcApiImplMock);

        ON_CALL(service, ConfigLine())
            .WillByDefault(Return("{\"root\":{\"mode\":\"Off\"}}"));
        ON_CALL(service, WebPrefix())
            .WillByDefault(Return(webPrefix));
#if 0
        ON_CALL(service, SubSystems())
            .WillByDefault(Invoke(
                [&]() {
                    PluginHost::ISubSystem* result = (&subSystem);
                    result->AddRef();
                    return result;
                }));
#endif
        EXPECT_EQ(string(""), plugin->Initialize(&service));
    }

    virtual ~DeviceInfoTest()
    {
        plugin->Deinitialize(&service);

        RfcApi::setImpl(nullptr);
        if (p_rfcApiImplMock != nullptr) {
            delete p_rfcApiImplMock;
            p_rfcApiImplMock = nullptr;
        }

        device::VideoOutputPortType::setImpl(nullptr);
        if (p_videoOutputPortTypeMock != nullptr) {
            delete p_videoOutputPortTypeMock;
            p_videoOutputPortTypeMock = nullptr;
        }

        device::VideoOutputPortConfig::setImpl(nullptr);
        if (p_videoOutputPortConfigImplMock != nullptr) {
            delete p_videoOutputPortConfigImplMock;
            p_videoOutputPortConfigImplMock = nullptr;
        }

        device::VideoOutputPort::setImpl(nullptr);
        if (p_videoOutputPortMock != nullptr) {
            delete p_videoOutputPortMock;
            p_videoOutputPortMock = nullptr;
        }

        device::VideoResolution::setImpl(nullptr);
        if (p_videoResolutionMock != nullptr) {
            delete p_videoResolutionMock;
            p_videoResolutionMock = nullptr;
        }

        device::AudioOutputPort::setImpl(nullptr);
        if (p_audioOutputPortMock != nullptr) {
            delete p_audioOutputPortMock;
            p_audioOutputPortMock = nullptr;
        }

        device::Host::setImpl(nullptr);
        if (p_hostImplMock != nullptr) {
            delete p_hostImplMock;
            p_hostImplMock = nullptr;
        }

        device::Manager::setImpl(nullptr);
        if (p_managerImplMock != nullptr) {
            delete p_managerImplMock;
            p_managerImplMock = nullptr;
        }

        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr) {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
    }

    void SetUpMFRCall(const char* expectedData, mfrSerializedType_t type)
    {
        ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
            .WillByDefault(Invoke(
                [expectedData, type](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                    if (string(ownerName) == string(_T(IARM_BUS_MFRLIB_NAME)) &&
                        string(methodName) == string(_T(IARM_BUS_MFRLIB_API_GetSerializedData))) {
                        auto* param = static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
                        if (param->type == type) {
                            param->bufLen = strlen(expectedData);
                            strncpy(param->buffer, expectedData, sizeof(param->buffer));
                            return IARM_RESULT_SUCCESS;
                        }
                    }
                    return IARM_RESULT_INVALID_PARAM;
                }));
    }

    void SetUpRFCCall(const char* expectedValue, const char* paramName)
    {
        ON_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
            .WillByDefault(Invoke(
                [expectedValue, paramName](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                    if (string(pcParameterName) == string(paramName)) {
                        strncpy(pstParamData->value, expectedValue, sizeof(pstParamData->value));
                        return WDMP_SUCCESS;
                    }
                    return WDMP_FAILURE;
                }));
    }
};

TEST_F(DeviceInfoTest, SerialNumber_Success_FromMFR)
{
    SetUpMFRCall("TEST12345", mfrSERIALIZED_TYPE_SERIALNUMBER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
    EXPECT_EQ(response, _T("{\"serialnumber\":\"TEST12345\"}"));
}

TEST_F(DeviceInfoTest, SerialNumber_Success_FromRFC)
{
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));
    
    SetUpRFCCall("RFC12345", "Device.DeviceInfo.SerialNumber");

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
    EXPECT_EQ(response, _T("{\"serialnumber\":\"RFC12345\"}"));
}

TEST_F(DeviceInfoTest, SerialNumber_Failure_BothSourcesFail)
{
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));
    
    ON_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
        .WillByDefault(Return(WDMP_FAILURE));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
}

TEST_F(DeviceInfoTest, Sku_Success_FromFile)
{
    std::ofstream file("/etc/device.properties");
    file << "MODEL_NUM=SKU-TEST-001\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelid"), _T(""), response));
    EXPECT_EQ(response, _T("{\"sku\":\"SKU-TEST-001\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Sku_Success_FromMFR)
{
    remove("/etc/device.properties");
    SetUpMFRCall("MFR-SKU-002", mfrSERIALIZED_TYPE_MODELNAME);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelid"), _T(""), response));
    EXPECT_EQ(response, _T("{\"sku\":\"MFR-SKU-002\"}"));
}

TEST_F(DeviceInfoTest, Sku_Success_FromRFC)
{
    remove("/etc/device.properties");
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));
    
    SetUpRFCCall("RFC-SKU-003", "Device.DeviceInfo.ModelName");

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelid"), _T(""), response));
    EXPECT_EQ(response, _T("{\"sku\":\"RFC-SKU-003\"}"));
}

TEST_F(DeviceInfoTest, Sku_Failure_AllSourcesFail)
{
    remove("/etc/device.properties");
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));
    
    ON_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
        .WillByDefault(Return(WDMP_FAILURE));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("modelid"), _T(""), response));
}

TEST_F(DeviceInfoTest, Make_Success_FromMFR)
{
    SetUpMFRCall("TestManufacturer", mfrSERIALIZED_TYPE_MANUFACTURER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("make"), _T(""), response));
    EXPECT_EQ(response, _T("{\"make\":\"TestManufacturer\"}"));
}

TEST_F(DeviceInfoTest, Make_Success_FromFile)
{
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));

    std::ofstream file("/etc/device.properties");
    file << "MFG_NAME=FileManufacturer\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("make"), _T(""), response));
    EXPECT_EQ(response, _T("{\"make\":\"FileManufacturer\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Make_Failure_BothSourcesFail)
{
    remove("/etc/device.properties");
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("make"), _T(""), response));
}

TEST_F(DeviceInfoTest, Model_Success_FromFile)
{
    std::ofstream file("/etc/device.properties");
    file << "FRIENDLY_ID=TestModel123\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelname"), _T(""), response));
    EXPECT_EQ(response, _T("{\"model\":\"TestModel123\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Model_Success_WithQuotes)
{
    std::ofstream file("/etc/device.properties");
    file << "FRIENDLY_ID=\"Test Model 456\"\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelname"), _T(""), response));
    EXPECT_EQ(response, _T("{\"model\":\"Test Model 456\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Model_Failure_FileNotFound)
{
    remove("/etc/device.properties");

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("modelname"), _T(""), response));
}

TEST_F(DeviceInfoTest, DeviceType_Success_IpTv)
{
    std::ofstream file("/etc/authService.conf");
    file << "deviceType=IpTv\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"IpTv\"}"));

    remove("/etc/authService.conf");
}

TEST_F(DeviceInfoTest, DeviceType_Success_IpStb)
{
    std::ofstream file("/etc/authService.conf");
    file << "deviceType=IpStb\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"IpStb\"}"));

    remove("/etc/authService.conf");
}

TEST_F(DeviceInfoTest, DeviceType_Success_QamIpStb)
{
    std::ofstream file("/etc/authService.conf");
    file << "deviceType=QamIpStb\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"QamIpStb\"}"));

    remove("/etc/authService.conf");
}

TEST_F(DeviceInfoTest, DeviceType_Success_FromDeviceProperties_MediaClient)
{
    remove("/etc/authService.conf");
    std::ofstream file("/etc/device.properties");
    file << "DEVICE_TYPE=mediaclient\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"IpStb\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, DeviceType_Success_FromDeviceProperties_Hybrid)
{
    remove("/etc/authService.conf");
    std::ofstream file("/etc/device.properties");
    file << "DEVICE_TYPE=hybrid\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"QamIpStb\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, DeviceType_Success_FromDeviceProperties_Other)
{
    remove("/etc/authService.conf");
    std::ofstream file("/etc/device.properties");
    file << "DEVICE_TYPE=other\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"IpTv\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, DeviceType_Failure_BothFilesNotFound)
{
    remove("/etc/authService.conf");
    remove("/etc/device.properties");

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("devicetype"), _T(""), response));
}

TEST_F(DeviceInfoTest, SocName_Success)
{
    std::ofstream file("/etc/device.properties");
    file << "SOC=BCM7218\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("socname"), _T(""), response));
    EXPECT_EQ(response, _T("{\"socname\":\"BCM7218\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, SocName_Failure_FileNotFound)
{
    remove("/etc/device.properties");

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("socname"), _T(""), response));
}

TEST_F(DeviceInfoTest, DistributorId_Success_FromFile)
{
    std::ofstream file("/opt/www/authService/partnerId3.dat");
    file << "PARTNER123\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("distributorid"), _T(""), response));
    EXPECT_EQ(response, _T("{\"distributorid\":\"PARTNER123\"}"));

    remove("/opt/www/authService/partnerId3.dat");
}

TEST_F(DeviceInfoTest, DistributorId_Success_FromRFC)
{
    remove("/opt/www/authService/partnerId3.dat");
    SetUpRFCCall("RFCPARTNER456", "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId");

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("distributorid"), _T(""), response));
    EXPECT_EQ(response, _T("{\"distributorid\":\"RFCPARTNER456\"}"));
}

TEST_F(DeviceInfoTest, DistributorId_Failure_BothSourcesFail)
{
    remove("/opt/www/authService/partnerId3.dat");
    ON_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
        .WillByDefault(Return(WDMP_FAILURE));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("distributorid"), _T(""), response));
}

TEST_F(DeviceInfoTest, Brand_Success_FromTmpFile)
{
    std::ofstream file("/tmp/.manufacturer");
    file << "TestBrand\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("brandname"), _T(""), response));
    EXPECT_EQ(response, _T("{\"brand\":\"TestBrand\"}"));

    remove("/tmp/.manufacturer");
}

TEST_F(DeviceInfoTest, Brand_Success_FromMFR)
{
    remove("/tmp/.manufacturer");
    SetUpMFRCall("MFRBrand", mfrSERIALIZED_TYPE_MANUFACTURER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("brandname"), _T(""), response));
    EXPECT_EQ(response, _T("{\"brand\":\"MFRBrand\"}"));
}

TEST_F(DeviceInfoTest, Brand_Failure_BothSourcesFail)
{
    remove("/tmp/.manufacturer");
    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("brandname"), _T(""), response));
}

TEST_F(DeviceInfoTest, ReleaseVersion_Success_ValidPattern)
{
    std::ofstream file("/version.txt");
    file << "imagename:VBN_2203_sprint_20220331225312sdy_NG\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"22.3.0.0\"}"));

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, ReleaseVersion_Success_AnotherValidPattern)
{
    std::ofstream file("/version.txt");
    file << "imagename:TEST_1805p_stable\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"18.5.0.0\"}"));

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, ReleaseVersion_DefaultVersion_InvalidPattern)
{
    std::ofstream file("/version.txt");
    file << "imagename:INVALID_VERSION_STRING\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"99.99.0.0\"}"));

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, ReleaseVersion_DefaultVersion_FileNotFound)
{
    remove("/version.txt");

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"99.99.0.0\"}"));
}

TEST_F(DeviceInfoTest, ChipSet_Success)
{
    std::ofstream file("/etc/device.properties");
    file << "CHIPSET_NAME=BCM7252S\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("chipset"), _T(""), response));
    EXPECT_EQ(response, _T("{\"chipset\":\"BCM7252S\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, ChipSet_Failure_FileNotFound)
{
    remove("/etc/device.properties");

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("chipset"), _T(""), response));
}

TEST_F(DeviceInfoTest, FirmwareVersion_Success_AllFields)
{
    std::ofstream file("/version.txt");
    file << "imagename:TEST_IMAGE_V1\n";
    file << "SDK_VERSION=18.4\n";
    file << "MEDIARITE=9.0.1\n";
    file << "YOCTO_VERSION=dunfell\n";
    file.close();

    SetUpMFRCall("PDRI_1.2.3", mfrSERIALIZED_TYPE_PDRIVERSION);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("firmwareversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"imagename\":\"TEST_IMAGE_V1\",\"sdk\":\"18.4\",\"mediarite\":\"9.0.1\",\"yocto\":\"dunfell\",\"pdri\":\"PDRI_1.2.3\"}"));

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, FirmwareVersion_Success_MissingOptionalFields)
{
    std::ofstream file("/version.txt");
    file << "imagename:TEST_IMAGE_V2\n";
    file.close();

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("firmwareversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"imagename\":\"TEST_IMAGE_V2\",\"sdk\":\"\",\"mediarite\":\"\",\"yocto\":\"\",\"pdri\":\"\"}"));

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, FirmwareVersion_Failure_ImageNameNotFound)
{
    std::ofstream file("/version.txt");
    file << "SDK_VERSION=18.4\n";
    file.close();

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("firmwareversion"), _T(""), response));

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, FirmwareVersion_Failure_FileNotFound)
{
    remove("/version.txt");

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("firmwareversion"), _T(""), response));
}

TEST_F(DeviceInfoTest, DISABLE_SystemInfo_Success)
{
    SetUpMFRCall("SYSTEMSERIAL123", mfrSERIALIZED_TYPE_SERIALNUMBER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("systeminfo"), _T(""), response));
    EXPECT_TRUE(response.find("\"version\":") != string::npos);
    EXPECT_TRUE(response.find("\"uptime\":") != string::npos);
    EXPECT_TRUE(response.find("\"totalram\":") != string::npos);
    EXPECT_TRUE(response.find("\"freeram\":") != string::npos);
    EXPECT_TRUE(response.find("\"devicename\":") != string::npos);
    EXPECT_TRUE(response.find("\"cpuload\":") != string::npos);
    EXPECT_TRUE(response.find("\"serialnumber\":\"SYSTEMSERIAL123\"") != string::npos);
    EXPECT_TRUE(response.find("\"time\":") != string::npos);
}

TEST_F(DeviceInfoTest, Addresses_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("addresses"), _T(""), response));
    EXPECT_TRUE(response.find("\"name\":") != string::npos);
    EXPECT_TRUE(response.find("\"mac\":") != string::npos);
}

TEST_F(DeviceInfoTest, SupportedAudioPorts_Success)
{
    device::List<device::AudioOutputPort> audioPorts;
    device::AudioOutputPort port1, port2;
    static const string portName1 = "HDMI0";
    static const string portName2 = "SPDIF";

    EXPECT_CALL(*p_audioOutputPortMock, getName())
        .WillOnce(ReturnRef(portName1))
        .WillOnce(ReturnRef(portName2));

    EXPECT_CALL(*p_hostImplMock, getAudioOutputPorts())
        .WillOnce(Invoke([&]() {
            audioPorts.push_back(port1);
            audioPorts.push_back(port2);
            return audioPorts;
        }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("supportedaudioports"), _T(""), response));
    EXPECT_TRUE(response.find("\"supportedAudioPorts\":[") != string::npos);
    EXPECT_TRUE(response.find("\"success\":true") != string::npos);
}

TEST_F(DeviceInfoTest, SupportedAudioPorts_Exception_DeviceException)
{
    EXPECT_CALL(*p_hostImplMock, getAudioOutputPorts())
        .WillOnce(Invoke([]() -> device::List<device::AudioOutputPort> {
            throw device::Exception("Test exception");
        }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("supportedaudioports"), _T(""), response));
}

TEST_F(DeviceInfoTest, SupportedAudioPorts_Exception_StdException)
{
    EXPECT_CALL(*p_hostImplMock, getAudioOutputPorts())
        .WillOnce(Invoke([]() -> device::List<device::AudioOutputPort> {
            throw std::runtime_error("Test exception");
        }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("supportedaudioports"), _T(""), response));
}

TEST_F(DeviceInfoTest, SupportedAudioPorts_Exception_UnknownException)
{
    EXPECT_CALL(*p_hostImplMock, getAudioOutputPorts())
        .WillOnce(Invoke([]() -> device::List<device::AudioOutputPort> {
            throw 42;
        }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("supportedaudioports"), _T(""), response));
}

// =========== Additional Negative Tests ===========

TEST_F(DeviceInfoTest, SerialNumber_Negative_MFRCallThrowsException)
{
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Invoke(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                throw std::runtime_error("IARM_Bus_Call exception");
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(DeviceInfoTest, SerialNumber_Negative_RFCThrowsException)
{
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Return(IARM_RESULT_INVALID_PARAM));

    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
        .WillOnce(Invoke(
            [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                throw device::Exception("getRFCParameter exception");
                return WDMP_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(DeviceInfoTest, SerialNumber_Negative_EmptyBuffer)
{
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Invoke(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                auto* param = static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
                param->bufLen = 0;
                param->buffer[0] = '\0';
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
        .WillOnce(Return(WDMP_FAILURE));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(DeviceInfoTest, Sku_Negative_FileReadException)
{
    remove("/etc/device.properties");

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Invoke(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                throw device::Exception("IARM exception");
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("modelid"), _T(""), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(DeviceInfoTest, Sku_Negative_InvalidFileFormat)
{
    std::ofstream file("/etc/device.properties");
    file << "INVALID_FORMAT_LINE\n";
    file << "MODEL_NUM_WITHOUT_EQUAL_SIGN\n";
    file.close();

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Return(IARM_RESULT_INVALID_PARAM));

    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
        .WillOnce(Return(WDMP_FAILURE));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("modelid"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Make_Negative_MFRThrowsException)
{
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Invoke(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                throw std::runtime_error("MFR exception");
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("make"), _T(""), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(DeviceInfoTest, Make_Negative_InvalidFileFormat)
{
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Return(IARM_RESULT_INVALID_PARAM));

    std::ofstream file("/etc/device.properties");
    file << "MFG_NAME_NO_VALUE=\n";
    file << "OTHER_KEY=value\n";
    file.close();

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("make"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Model_Negative_EmptyFriendlyId)
{
    std::ofstream file("/etc/device.properties");
    file << "FRIENDLY_ID=\n";
    file.close();

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("modelname"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Model_Negative_FileAccessException)
{
    remove("/etc/device.properties");

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("modelname"), _T(""), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(DeviceInfoTest, DeviceType_Negative_InvalidDeviceType)
{
    std::ofstream file("/etc/authService.conf");
    file << "deviceType=InvalidType\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));

    remove("/etc/authService.conf");
}

TEST_F(DeviceInfoTest, DeviceType_Negative_EmptyDeviceType)
{
    std::ofstream file("/etc/authService.conf");
    file << "deviceType=\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));

    remove("/etc/authService.conf");
}

TEST_F(DeviceInfoTest, DeviceType_Negative_MalformedFile)
{
    std::ofstream file("/etc/authService.conf");
    file << "INVALID LINE FORMAT\n";
    file << "NO EQUAL SIGN\n";
    file.close();

    remove("/etc/device.properties");

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/etc/authService.conf");
}

TEST_F(DeviceInfoTest, SocName_Negative_EmptySocValue)
{
    std::ofstream file("/etc/device.properties");
    file << "SOC=\n";
    file.close();

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("socname"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, SocName_Negative_MalformedFile)
{
    std::ofstream file("/etc/device.properties");
    file << "SOC_WITHOUT_VALUE\n";
    file << "OTHER_KEY=value\n";
    file.close();

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("socname"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, DistributorId_Negative_EmptyFile)
{
    std::ofstream file("/opt/www/authService/partnerId3.dat");
    file << "";
    file.close();

    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
        .WillOnce(Return(WDMP_FAILURE));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("distributorid"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/opt/www/authService/partnerId3.dat");
}

TEST_F(DeviceInfoTest, DistributorId_Negative_RFCThrowsException)
{
    remove("/opt/www/authService/partnerId3.dat");

    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
        .WillOnce(Invoke(
            [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                throw std::runtime_error("RFC exception");
                return WDMP_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("distributorid"), _T(""), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(DeviceInfoTest, Brand_Negative_BothSourcesEmpty)
{
    std::ofstream file("/tmp/.manufacturer");
    file << "";
    file.close();

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Return(IARM_RESULT_INVALID_PARAM));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("brandname"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/tmp/.manufacturer");
}

TEST_F(DeviceInfoTest, Brand_Negative_MFRThrowsException)
{
    remove("/tmp/.manufacturer");

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Invoke(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                throw device::Exception("MFR exception");
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("brandname"), _T(""), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(DeviceInfoTest, ReleaseVersion_Negative_MalformedImageName)
{
    std::ofstream file("/version.txt");
    file << "imagename:\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"99.99.0.0\"}"));

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, ReleaseVersion_Negative_SpecialCharacters)
{
    std::ofstream file("/version.txt");
    file << "imagename:ABC@#$%XYZ\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"99.99.0.0\"}"));

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, ChipSet_Negative_EmptyChipsetValue)
{
    std::ofstream file("/etc/device.properties");
    file << "CHIPSET_NAME=\n";
    file.close();

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("chipset"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, ChipSet_Negative_MalformedFile)
{
    std::ofstream file("/etc/device.properties");
    file << "CHIPSET_NAME_NO_VALUE\n";
    file << "RANDOM_DATA\n";
    file.close();

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("chipset"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, FirmwareVersion_Negative_EmptyImageName)
{
    std::ofstream file("/version.txt");
    file << "imagename:\n";
    file << "SDK_VERSION=18.4\n";
    file.close();

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("firmwareversion"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, FirmwareVersion_Negative_PDRICallThrowsException)
{
    std::ofstream file("/version.txt");
    file << "imagename:TEST_IMAGE\n";
    file.close();

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Invoke(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                throw std::runtime_error("PDRI exception");
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("firmwareversion"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, FirmwareVersion_Negative_MalformedVersionFile)
{
    std::ofstream file("/version.txt");
    file << "INVALID_FORMAT\n";
    file << "NO_IMAGENAME_KEY\n";
    file.close();

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("firmwareversion"), _T(""), response));
    EXPECT_TRUE(response.empty());

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, DISABLE_SystemInfo_Negative_SerialNumberFails)
{
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillRepeatedly(Return(IARM_RESULT_INVALID_PARAM));

    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
        .WillRepeatedly(Return(WDMP_FAILURE));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("systeminfo"), _T(""), response));
    EXPECT_TRUE(response.find("\"serialnumber\":\"\"") != string::npos);
}

TEST_F(DeviceInfoTest, DISABLE_SystemInfo_Negative_MFRThrowsException)
{
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Invoke(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                throw device::Exception("System info exception");
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("systeminfo"), _T(""), response));
    EXPECT_TRUE(response.find("\"serialnumber\":\"\"") != string::npos);
}

TEST_F(DeviceInfoTest, SupportedAudioPorts_Negative_GetNameThrowsException)
{
    device::List<device::AudioOutputPort> audioPorts;
    device::AudioOutputPort port1;

    EXPECT_CALL(*p_audioOutputPortMock, getName())
        .WillOnce(Invoke([]() -> const string& {
            throw device::Exception("getName exception");
            static const string portName = "HDMI0";
            return portName;
        }));

    EXPECT_CALL(*p_hostImplMock, getAudioOutputPorts())
        .WillOnce(Invoke([&]() {
            audioPorts.push_back(port1);
            return audioPorts;
        }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("supportedaudioports"), _T(""), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(DeviceInfoTest, SupportedAudioPorts_Negative_EmptyPortList)
{
    device::List<device::AudioOutputPort> audioPorts;

    EXPECT_CALL(*p_hostImplMock, getAudioOutputPorts())
        .WillOnce(Return(audioPorts));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("supportedaudioports"), _T(""), response));
    EXPECT_TRUE(response.find("\"supportedAudioPorts\":[]") != string::npos);
}

// =========== Additional Comprehensive Positive Tests ===========

TEST_F(DeviceInfoTest, SerialNumber_Positive_LongSerialNumber)
{
    std::string longSerial(255, 'X');
    
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Invoke(
            [&longSerial](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                auto* param = static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
                param->bufLen = longSerial.length();
                strncpy(param->buffer, longSerial.c_str(), sizeof(param->buffer));
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
    EXPECT_TRUE(response.find(longSerial) != string::npos);
}

TEST_F(DeviceInfoTest, SerialNumber_Positive_SpecialCharacters)
{
    SetUpMFRCall("SN-123_ABC.DEF#456", mfrSERIALIZED_TYPE_SERIALNUMBER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
    EXPECT_EQ(response, _T("{\"serialnumber\":\"SN-123_ABC.DEF#456\"}"));
}

TEST_F(DeviceInfoTest, SerialNumber_Positive_NumericOnly)
{
    SetUpMFRCall("1234567890", mfrSERIALIZED_TYPE_SERIALNUMBER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
    EXPECT_EQ(response, _T("{\"serialnumber\":\"1234567890\"}"));
}

TEST_F(DeviceInfoTest, Sku_Positive_FileWithSpaces)
{
    std::ofstream file("/etc/device.properties");
    file << "MODEL_NUM  =  SKU WITH SPACES  \n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelid"), _T(""), response));
    EXPECT_TRUE(response.find("SKU WITH SPACES") != string::npos);

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Sku_Positive_FileWithQuotes)
{
    std::ofstream file("/etc/device.properties");
    file << "MODEL_NUM=\"QUOTED-SKU-001\"\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelid"), _T(""), response));
    EXPECT_TRUE(response.find("QUOTED-SKU-001") != string::npos);

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Sku_Positive_MultipleLines)
{
    std::ofstream file("/etc/device.properties");
    file << "OTHER_KEY=value1\n";
    file << "MODEL_NUM=CORRECT_SKU\n";
    file << "ANOTHER_KEY=value2\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelid"), _T(""), response));
    EXPECT_EQ(response, _T("{\"sku\":\"CORRECT_SKU\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Make_Positive_SpecialCharacters)
{
    SetUpMFRCall("ACME & Co.", mfrSERIALIZED_TYPE_MANUFACTURER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("make"), _T(""), response));
    EXPECT_EQ(response, _T("{\"make\":\"ACME & Co.\"}"));
}

TEST_F(DeviceInfoTest, Make_Positive_LongName)
{
    std::string longName(200, 'M');
    
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Invoke(
            [&longName](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                auto* param = static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
                param->bufLen = longName.length();
                strncpy(param->buffer, longName.c_str(), sizeof(param->buffer));
                return IARM_RESULT_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("make"), _T(""), response));
    EXPECT_TRUE(response.find(longName) != string::npos);
}

TEST_F(DeviceInfoTest, Model_Positive_MultiWordModel)
{
    std::ofstream file("/etc/device.properties");
    file << "FRIENDLY_ID=\"Ultra HD Smart TV 2024\"\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelname"), _T(""), response));
    EXPECT_EQ(response, _T("{\"model\":\"Ultra HD Smart TV 2024\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Model_Positive_AlphanumericWithDash)
{
    std::ofstream file("/etc/device.properties");
    file << "FRIENDLY_ID=STB-X1-2024\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelname"), _T(""), response));
    EXPECT_EQ(response, _T("{\"model\":\"STB-X1-2024\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, DeviceType_Positive_AllValidTypes)
{
    // Test IpTv
    std::ofstream file1("/etc/authService.conf");
    file1 << "deviceType=IpTv\n";
    file1.close();
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"IpTv\"}"));
    remove("/etc/authService.conf");

    // Test IpStb
    std::ofstream file2("/etc/authService.conf");
    file2 << "deviceType=IpStb\n";
    file2.close();
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"IpStb\"}"));
    remove("/etc/authService.conf");

    // Test QamIpStb
    std::ofstream file3("/etc/authService.conf");
    file3 << "deviceType=QamIpStb\n";
    file3.close();
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"QamIpStb\"}"));
    remove("/etc/authService.conf");
}

TEST_F(DeviceInfoTest, DeviceType_Positive_FallbackToDeviceProperties)
{
    remove("/etc/authService.conf");
    
    std::ofstream file("/etc/device.properties");
    file << "DEVICE_TYPE=mediaclient\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"IpStb\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, SocName_Positive_AlphanumericSoC)
{
    std::ofstream file("/etc/device.properties");
    file << "SOC=BCM7218_V30\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("socname"), _T(""), response));
    EXPECT_EQ(response, _T("{\"socname\":\"BCM7218_V30\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, SocName_Positive_WithSpaces)
{
    std::ofstream file("/etc/device.properties");
    file << "SOC  =  BCM7271  \n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("socname"), _T(""), response));
    EXPECT_TRUE(response.find("BCM7271") != string::npos);

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, DistributorId_Positive_AlphanumericId)
{
    std::ofstream file("/opt/www/authService/partnerId3.dat");
    file << "PARTNER-123-XYZ\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("distributorid"), _T(""), response));
    EXPECT_EQ(response, _T("{\"distributorid\":\"PARTNER-123-XYZ\"}"));

    remove("/opt/www/authService/partnerId3.dat");
}

TEST_F(DeviceInfoTest, DistributorId_Positive_RFCWithSpecialChars)
{
    remove("/opt/www/authService/partnerId3.dat");
    SetUpRFCCall("RFC_DIST_ID.001", "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId");

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("distributorid"), _T(""), response));
    EXPECT_EQ(response, _T("{\"distributorid\":\"RFC_DIST_ID.001\"}"));
}

TEST_F(DeviceInfoTest, Brand_Positive_FileWithWhitespace)
{
    std::ofstream file("/tmp/.manufacturer");
    file << "  BrandName  \n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("brandname"), _T(""), response));
    EXPECT_TRUE(response.find("BrandName") != string::npos);

    remove("/tmp/.manufacturer");
}

TEST_F(DeviceInfoTest, Brand_Positive_MFRFallback)
{
    remove("/tmp/.manufacturer");
    SetUpMFRCall("FallbackBrand", mfrSERIALIZED_TYPE_MANUFACTURER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("brandname"), _T(""), response));
    EXPECT_EQ(response, _T("{\"brand\":\"FallbackBrand\"}"));
}

TEST_F(DeviceInfoTest, ReleaseVersion_Positive_VariousPatterns)
{
    // Test pattern 1: YYMM with 'p'
    std::ofstream file1("/version.txt");
    file1 << "imagename:VBN_2201p_stable\n";
    file1.close();
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"22.1.0.0\"}"));
    remove("/version.txt");

    // Test pattern 2: YYMM with 's'
    std::ofstream file2("/version.txt");
    file2 << "imagename:TEST_1912s_test\n";
    file2.close();
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"19.12.0.0\"}"));
    remove("/version.txt");

    // Test pattern 3: Single digit month
    std::ofstream file3("/version.txt");
    file3 << "imagename:BUILD_2305p_final\n";
    file3.close();
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"23.5.0.0\"}"));
    remove("/version.txt");
}

TEST_F(DeviceInfoTest, ReleaseVersion_Positive_DefaultVersion)
{
    std::ofstream file("/version.txt");
    file << "imagename:NO_VERSION_PATTERN\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"99.99.0.0\"}"));

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, ChipSet_Positive_AlphanumericChipset)
{
    std::ofstream file("/etc/device.properties");
    file << "CHIPSET_NAME=BCM7252S-B0\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("chipset"), _T(""), response));
    EXPECT_EQ(response, _T("{\"chipset\":\"BCM7252S-B0\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, ChipSet_Positive_WithQuotes)
{
    std::ofstream file("/etc/device.properties");
    file << "CHIPSET_NAME=\"BCM7271T\"\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("chipset"), _T(""), response));
    EXPECT_TRUE(response.find("BCM7271T") != string::npos);

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, FirmwareVersion_Positive_PartialFields)
{
    std::ofstream file("/version.txt");
    file << "imagename:PARTIAL_IMAGE\n";
    file << "SDK_VERSION=20.1\n";
    file.close();

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("firmwareversion"), _T(""), response));
    EXPECT_TRUE(response.find("\"imagename\":\"PARTIAL_IMAGE\"") != string::npos);
    EXPECT_TRUE(response.find("\"sdk\":\"20.1\"") != string::npos);
    EXPECT_TRUE(response.find("\"mediarite\":\"\"") != string::npos);

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, FirmwareVersion_Positive_AllOptionalFields)
{
    std::ofstream file("/version.txt");
    file << "imagename:FULL_IMAGE_V3\n";
    file << "SDK_VERSION=21.2\n";
    file << "MEDIARITE=10.1.5\n";
    file << "YOCTO_VERSION=kirkstone\n";
    file.close();

    SetUpMFRCall("PDRI_2.3.4", mfrSERIALIZED_TYPE_PDRIVERSION);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("firmwareversion"), _T(""), response));
    EXPECT_TRUE(response.find("\"imagename\":\"FULL_IMAGE_V3\"") != string::npos);
    EXPECT_TRUE(response.find("\"sdk\":\"21.2\"") != string::npos);
    EXPECT_TRUE(response.find("\"mediarite\":\"10.1.5\"") != string::npos);
    EXPECT_TRUE(response.find("\"yocto\":\"kirkstone\"") != string::npos);
    EXPECT_TRUE(response.find("\"pdri\":\"PDRI_2.3.4\"") != string::npos);

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, DISABLE_SystemInfo_Positive_AllFieldsPresent)
{
    SetUpMFRCall("SYSSERIAL999", mfrSERIALIZED_TYPE_SERIALNUMBER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("systeminfo"), _T(""), response));
    EXPECT_TRUE(response.find("\"version\":") != string::npos);
    EXPECT_TRUE(response.find("\"uptime\":") != string::npos);
    EXPECT_TRUE(response.find("\"totalram\":") != string::npos);
    EXPECT_TRUE(response.find("\"freeram\":") != string::npos);
    EXPECT_TRUE(response.find("\"devicename\":") != string::npos);
    EXPECT_TRUE(response.find("\"cpuload\":") != string::npos);
    EXPECT_TRUE(response.find("\"serialnumber\":\"SYSSERIAL999\"") != string::npos);
    EXPECT_TRUE(response.find("\"time\":") != string::npos);
}

TEST_F(DeviceInfoTest, Addresses_Positive_HasRequiredFields)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("addresses"), _T(""), response));
    EXPECT_TRUE(response.find("\"name\":") != string::npos);
    EXPECT_TRUE(response.find("\"mac\":") != string::npos);
}

TEST_F(DeviceInfoTest, SupportedAudioPorts_Positive_MultiplePortTypes)
{
    device::List<device::AudioOutputPort> audioPorts;
    device::AudioOutputPort port1, port2, port3;
    static const string portName1 = "HDMI0";
    static const string portName2 = "SPDIF";
    static const string portName3 = "SPEAKER";

    EXPECT_CALL(*p_audioOutputPortMock, getName())
        .WillOnce(ReturnRef(portName1))
        .WillOnce(ReturnRef(portName2))
        .WillOnce(ReturnRef(portName3));

    EXPECT_CALL(*p_hostImplMock, getAudioOutputPorts())
        .WillOnce(Invoke([&]() {
            audioPorts.push_back(port1);
            audioPorts.push_back(port2);
            audioPorts.push_back(port3);
            return audioPorts;
        }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("supportedaudioports"), _T(""), response));
    EXPECT_TRUE(response.find("\"HDMI0\"") != string::npos);
    EXPECT_TRUE(response.find("\"SPDIF\"") != string::npos);
TEST_F(DeviceInfoTest, SupportedAudioPorts_Positive_SinglePort)
{
    device::List<device::AudioOutputPort> audioPorts;
    device::AudioOutputPort port1;
    static const string portName = "HDMI0";

    EXPECT_CALL(*p_audioOutputPortMock, getName())
        .WillOnce(ReturnRef(portName));

    EXPECT_CALL(*p_hostImplMock, getAudioOutputPorts())
        .WillOnce(Invoke([&]() {
            audioPorts.push_back(port1);
            return audioPorts;
        }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("supportedaudioports"), _T(""), response));
    EXPECT_TRUE(response.find("\"supportedAudioPorts\":[") != string::npos);
    EXPECT_TRUE(response.find("\"HDMI0\"") != string::npos);
}   EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("supportedaudioports"), _T(""), response));
    EXPECT_TRUE(response.find("\"supportedAudioPorts\":[") != string::npos);
    EXPECT_TRUE(response.find("\"HDMI0\"") != string::npos);
}

// =========== Boundary and Edge Case Tests ===========

TEST_F(DeviceInfoTest, Boundary_SerialNumber_MinLength)
{
    SetUpMFRCall("A", mfrSERIALIZED_TYPE_SERIALNUMBER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
    EXPECT_EQ(response, _T("{\"serialnumber\":\"A\"}"));
}

TEST_F(DeviceInfoTest, Boundary_Sku_EmptyModelNum)
{
    std::ofstream file("/etc/device.properties");
    file << "MODEL_NUM=\n";
    file.close();

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));

    ON_CALL(*p_rfcApiImplMock, getRFCParameter(_, _, _))
        .WillByDefault(Return(WDMP_FAILURE));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("modelid"), _T(""), response));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Boundary_Make_SingleCharacter)
{
    SetUpMFRCall("X", mfrSERIALIZED_TYPE_MANUFACTURER);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("make"), _T(""), response));
    EXPECT_EQ(response, _T("{\"make\":\"X\"}"));
}

TEST_F(DeviceInfoTest, Boundary_Model_VeryLongName)
{
    std::string longModel(500, 'M');
    std::ofstream file("/etc/device.properties");
    file << "FRIENDLY_ID=" << longModel << "\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("modelname"), _T(""), response));
    EXPECT_TRUE(response.find(longModel) != string::npos);

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, Boundary_ChipSet_SingleCharacter)
{
    std::ofstream file("/etc/device.properties");
    file << "CHIPSET_NAME=A\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("chipset"), _T(""), response));
    EXPECT_EQ(response, _T("{\"chipset\":\"A\"}"));

    remove("/etc/device.properties");
}

TEST_F(DeviceInfoTest, EdgeCase_ReleaseVersion_MissingFile)
{
    remove("/version.txt");

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("releaseversion"), _T(""), response));
    EXPECT_EQ(response, _T("{\"releaseversion\":\"99.99.0.0\"}"));
}

TEST_F(DeviceInfoTest, EdgeCase_FirmwareVersion_OnlyImageName)
{
    std::ofstream file("/version.txt");
    file << "imagename:MINIMAL_IMAGE\n";
    file.close();

    ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillByDefault(Return(IARM_RESULT_INVALID_PARAM));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("firmwareversion"), _T(""), response));
    EXPECT_TRUE(response.find("\"imagename\":\"MINIMAL_IMAGE\"") != string::npos);
    EXPECT_TRUE(response.find("\"sdk\":\"\"") != string::npos);
    EXPECT_TRUE(response.find("\"mediarite\":\"\"") != string::npos);
    EXPECT_TRUE(response.find("\"yocto\":\"\"") != string::npos);
    EXPECT_TRUE(response.find("\"pdri\":\"\"") != string::npos);

    remove("/version.txt");
}

TEST_F(DeviceInfoTest, EdgeCase_DeviceType_CaseSensitivity)
{
    std::ofstream file("/etc/authService.conf");
    file << "deviceType=IpStb\n";
    file.close();

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("devicetype"), _T(""), response));
    EXPECT_EQ(response, _T("{\"devicetype\":\"IpStb\"}"));

    remove("/etc/authService.conf");
}

TEST_F(DeviceInfoTest, EdgeCase_MultipleIARMCallsSequential)
{
    // Test multiple sequential MFR calls
    SetUpMFRCall("SERIAL001", mfrSERIALIZED_TYPE_SERIALNUMBER);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("serialnumber"), _T(""), response));
    EXPECT_EQ(response, _T("{\"serialnumber\":\"SERIAL001\"}"));

    SetUpMFRCall("MAKE001", mfrSERIALIZED_TYPE_MANUFACTURER);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("make"), _T(""), response));
    EXPECT_EQ(response, _T("{\"make\":\"MAKE001\"}"));

    SetUpMFRCall("BRAND001", mfrSERIALIZED_TYPE_MANUFACTURER);
    remove("/tmp/.manufacturer");
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("brandname"), _T(""), response));
    EXPECT_EQ(response, _T("{\"brand\":\"BRAND001\"}"));
}
