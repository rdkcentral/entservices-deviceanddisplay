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
#include <gmock/gmock.h>
#include "IarmBusMock.h"
#include "Iarm.h"
#include "mfrTypes.h"

#include "Implementation/FirmwareVersion.h"

#include <fstream>
#include "ThunderPortability.h"

using namespace WPEFramework;
using ::testing::NiceMock;

class FirmwareVersionTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::FirmwareVersion> firmwareVersion;
    Exchange::IFirmwareVersion* interface;
    IarmBusImplMock* p_iarmBusImplMock;

    FirmwareVersionTest()
        : firmwareVersion(Core::ProxyType<Plugin::FirmwareVersion>::Create())
    {
        interface = static_cast<Exchange::IFirmwareVersion*>(
            firmwareVersion->QueryInterface(Exchange::IFirmwareVersion::ID));
        p_iarmBusImplMock = new NiceMock<IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);
    }
    virtual ~FirmwareVersionTest()
    {
        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr)
        {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
        interface->Release();
    }

    virtual void SetUp()
    {
        ASSERT_TRUE(interface != nullptr);
    }

    virtual void TearDown()
    {
        ASSERT_TRUE(interface != nullptr);
    }
};

TEST_F(FirmwareVersionTest, Imagename)
{
    std::ofstream file("/version.txt");
    file << "imagename:CUSTOM5_VBN_2203_sprint_20220331225312sdy_NG";
    file.close();

    string imagename;
    EXPECT_EQ(Core::ERROR_NONE, interface->Imagename(imagename));
    EXPECT_EQ(imagename, _T("CUSTOM5_VBN_2203_sprint_20220331225312sdy_NG"));
}

TEST_F(FirmwareVersionTest, Sdk)
{
    std::ofstream file("/version.txt");
    file << "SDK_VERSION=17.3";
    file.close();

    string sdk;
    EXPECT_EQ(Core::ERROR_NONE, interface->Sdk(sdk));
    EXPECT_EQ(sdk, _T("17.3"));
}

TEST_F(FirmwareVersionTest, Mediarite)
{
    std::ofstream file("/version.txt");
    file << "MEDIARITE=8.3.53";
    file.close();

    string mediarite;
    EXPECT_EQ(Core::ERROR_NONE, interface->Mediarite(mediarite));
    EXPECT_EQ(mediarite, _T("8.3.53"));
}

TEST_F(FirmwareVersionTest, Yocto)
{
    std::ofstream file("/version.txt");
    file << "YOCTO_VERSION=dunfell";
    file.close();

    string yocto;
    EXPECT_EQ(Core::ERROR_NONE, interface->Yocto(yocto));
    EXPECT_EQ(yocto, _T("dunfell"));
}

TEST_F(FirmwareVersionTest, Pdri_Success)
{
    const char* expectedPdriVersion = "COESST11AEI_PDRI_PROD_20240119170804_3.2.2.0.bin";

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
        .WillRepeatedly(
            [&](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                EXPECT_STREQ(ownerName, "MFRLib");
                EXPECT_STREQ(methodName, "mfrGetManufacturerData");
                
                IARM_Bus_MFRLib_GetSerializedData_Param_t* param = 
                    static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
                
                // Verify the type is PDRI version
                EXPECT_EQ(param->type, mfrSERIALIZED_TYPE_PDRIVERSION);
                // Simulate successful response with bounds checking
                size_t expectedLen = strlen(expectedPdriVersion);
                if (expectedLen < sizeof(param->buffer)) {
                   strncpy(param->buffer, expectedPdriVersion, sizeof(param->buffer) - 1);
                   param->buffer[sizeof(param->buffer) - 1] = '\0';
                   param->bufLen = expectedLen;
                } else {
                   param->bufLen = 0;
                }
                                
                return IARM_RESULT_SUCCESS;
            });

    string pdri;
    EXPECT_EQ(Core::ERROR_NONE, interface->Pdri(pdri));
    EXPECT_EQ(pdri, expectedPdriVersion);
}

TEST_F(FirmwareVersionTest, Pdri_Failure)
{
    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(_, _, _, _))
        .WillOnce(Invoke([](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
            EXPECT_STREQ(ownerName, "MFRLib");
            EXPECT_STREQ(methodName, "mfrGetManufacturerData");
            
            IARM_Bus_MFRLib_GetSerializedData_Param_t* param = 
                static_cast<IARM_Bus_MFRLib_GetSerializedData_Param_t*>(arg);
            
            EXPECT_EQ(param->type, mfrSERIALIZED_TYPE_PDRIVERSION);
            
            // Simulate IARM call failure
            param->bufLen = 0;
            
            return IARM_RESULT_IPCCORE_FAIL;
        }));

    string pdri;
    EXPECT_EQ(Core::ERROR_GENERAL, interface->Pdri(pdri));
    EXPECT_TRUE(pdri.empty());
}
