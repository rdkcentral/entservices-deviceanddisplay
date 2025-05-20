/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
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

#include "gtest/gtest.h"
#include <gmock/gmock.h>

#include "Warehouse.h"

#include "FactoriesImplementation.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "RfcApiMock.h"
#include "WrapsMock.h"
#include "secure_wrappermock.h"
#include "ThunderPortability.h"

using namespace WPEFramework;

using ::testing::NiceMock;

class WarehouseTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::Warehouse> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    string response;

    WarehouseTest()
        : plugin(Core::ProxyType<Plugin::Warehouse>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
    {
    }

    virtual ~WarehouseTest()
    {
        plugin.Release();
    }
};

class WarehouseInitializedTest : public WarehouseTest {
protected:
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;
    RfcApiImplMock    *p_rfcApiImplMock  = nullptr ;
    WrapsImplMock     *p_wrapsImplMock   = nullptr ;

    IARM_EventHandler_t whMgrStatusChangeEventsHandler;

    WarehouseInitializedTest()
             : WarehouseTest()
    {
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        p_rfcApiImplMock  = new NiceMock <RfcApiImplMock>;
        RfcApi::setImpl(p_rfcApiImplMock);

        p_wrapsImplMock  = new NiceMock <WrapsImplMock>;
        Wraps::setImpl(p_wrapsImplMock);

        EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
            .Times(1)
            .WillOnce(::testing::Invoke(
                [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                    if ((string(IARM_BUS_PWRMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_PWRMGR_EVENT_WAREHOUSEOPS_STATUSCHANGED)) {
                        whMgrStatusChangeEventsHandler = handler;
                    }
                    return IARM_RESULT_SUCCESS;
                }));
        EXPECT_EQ(string(""), plugin->Initialize(nullptr));
    }
    virtual ~WarehouseInitializedTest() override
    {
        plugin->Deinitialize(nullptr);
        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr)
        {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }

        RfcApi::setImpl(nullptr);
        if (p_rfcApiImplMock != nullptr)
        {
            delete p_rfcApiImplMock;
            p_rfcApiImplMock = nullptr;
        }

        Wraps::setImpl(nullptr);
        if (p_wrapsImplMock != nullptr)
        {
            delete p_wrapsImplMock;
            p_wrapsImplMock = nullptr;
        }
    }
};

class WarehouseEventTest : public WarehouseInitializedTest {
protected:
    NiceMock<ServiceMock> service;
    NiceMock<FactoriesImplementation> factoriesImplementation;
    PLUGINHOST_DISPATCHER* dispatcher;
    Core::JSONRPC::Message message;

    WarehouseEventTest()
        : WarehouseInitializedTest()
    {
        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
           plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);
    }
    virtual ~WarehouseEventTest() override
    {
        dispatcher->Deactivate();
        dispatcher->Release();

        PluginHost::IFactories::Assign(nullptr);
    }
};

TEST_F(WarehouseTest, registeredMethods)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("resetDevice")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("internalReset")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("lightReset")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("isClean")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("executeHardwareTest")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getHardwareTestResults")));
}

class WarehouseResetDeviceTest : public WarehouseEventTest {
protected:
    Core::Event resetDone;
    WarehouseResetDeviceTest()
        : WarehouseEventTest(), resetDone(false, true)
    {
        EVENT_SUBSCRIBE(0, _T("resetDone"), _T("org.rdk.Warehouse"), message);

        EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
            .Times(1)
            // called by WareHouseResetIARM
            .WillOnce(::testing::Invoke(
                [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                    string text;
                    EXPECT_TRUE(json->ToString(text));
                    //Check for error response for the second call - Negative test case
                    EXPECT_EQ(text, string(_T("{"
                                            "\"jsonrpc\":\"2.0\","
                                            "\"method\":\"org.rdk.Warehouse.resetDone\","
                                            "\"params\":{\"success\":true}"
                                            "}")));
                    resetDone.SetEvent();
                    return Core::ERROR_NONE;
                }));
    }

    virtual ~WarehouseResetDeviceTest() override
    {
         EVENT_UNSUBSCRIBE(0, _T("resetDone"), _T("org.rdk.Warehouse"), message);
     }
};

class WarehouseResetDeviceFailureTest : public WarehouseEventTest {
protected:
    Core::Event resetDone;
    WarehouseResetDeviceFailureTest()
        : WarehouseEventTest(), resetDone(false, true)
    {
        EVENT_SUBSCRIBE(0, _T("resetDone"), _T("org.rdk.Warehouse"), message);

        EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
            .Times(1)
            // called by WareHouseResetIARM
            .WillOnce(::testing::Invoke(
                [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                    string text;
                    EXPECT_TRUE(json->ToString(text));
                    //Check for error response for the second call - Negative test case
                    EXPECT_EQ(text, string(_T("{"
                                            "\"jsonrpc\":\"2.0\","
                                            "\"method\":\"org.rdk.Warehouse.resetDone\","
                                            "\"params\":{\"success\":false,"
                                            "\"error\":\"Reset failed\"}"
                                            "}")));
                    resetDone.SetEvent();
                    return Core::ERROR_NONE;
                }));
}

    virtual ~WarehouseResetDeviceFailureTest() override
    {
         EVENT_UNSUBSCRIBE(0, _T("resetDone"), _T("org.rdk.Warehouse"), message);
     }
};

TEST_F(WarehouseResetDeviceTest, ColdFactoryResetDevice)
{
    EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_,::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke(
             [](const char *command, va_list args) {
                EXPECT_EQ(string(command), string("sh /lib/rdk/deviceReset.sh coldfactory"));
                return Core::ERROR_NONE;
            }))
        .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

    //reset: suppress reboot: true, type: COLD
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("resetDevice"), _T("{\"suppressReboot\":true,\"resetType\":\"COLD\"}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
    EXPECT_EQ(Core::ERROR_NONE, resetDone.Lock());
}

TEST_F(WarehouseResetDeviceTest, FactoryResetDevice)
{
    EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](const char *command, va_list args) {
                EXPECT_EQ(string(command), string("sh /lib/rdk/deviceReset.sh factory"));
                return Core::ERROR_NONE;
            }))
        .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

    //reset: suppress reboot: true, type: FACTORY
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("resetDevice"), _T("{\"suppressReboot\":true,\"resetType\":\"FACTORY\"}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
    EXPECT_EQ(Core::ERROR_NONE, resetDone.Lock());
}

TEST_F(WarehouseResetDeviceTest, UserFactoryResetDevice)
{
    EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](const char *command, va_list args) {
                EXPECT_EQ(string(command), string("sh /lib/rdk/deviceReset.sh userfactory"));
                return Core::ERROR_NONE;
            }));

    //reset: suppress reboot: true, type: USERFACTORY
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("resetDevice"), _T("{\"suppressReboot\":true,\"resetType\":\"USERFACTORY\"}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
    EXPECT_EQ(Core::ERROR_NONE, resetDone.Lock());
}

TEST_F(WarehouseResetDeviceTest, WarehouseClearResetDevice)
{
 EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](const char *command, va_list args) {
                EXPECT_EQ(string(command), string("sh /lib/rdk/deviceReset.sh WAREHOUSE_CLEAR"));
                return Core::ERROR_NONE;
            }));

    //reset: suppress reboot: false, type: WAREHOUSE_CLEAR
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("resetDevice"), _T("{\"suppressReboot\":false,\"resetType\":\"WAREHOUSE_CLEAR\"}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
    EXPECT_EQ(Core::ERROR_NONE, resetDone.Lock());
}

TEST_F(WarehouseInitializedTest, WarehouseClearResetDeviceNoResponse)
{
    Core::Event resetCallRxed(false, true);

        EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const char *command, va_list args) {
                EXPECT_EQ(string(command), string("sh /lib/rdk/deviceReset.sh WAREHOUSE_CLEAR --suppressReboot"));
                resetCallRxed.SetEvent();
                return Core::ERROR_NONE;
            }));

    //reset: suppress reboot: true, type: WAREHOUSE_CLEAR, Expect no response
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("resetDevice"), _T("{\"suppressReboot\":true,\"resetType\":\"WAREHOUSE_CLEAR\"}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
    EXPECT_EQ(Core::ERROR_NONE, resetCallRxed.Lock());
}

TEST_F(WarehouseResetDeviceTest, GenericResetDevice)
{

EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](const char *command, va_list args) {
                EXPECT_EQ(string(command), string("sh /lib/rdk/deviceReset.sh warehouse"));
                return Core::ERROR_NONE;
            }));

    //reset: suppress reboot: false
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("resetDevice"), _T("{\"suppressReboot\":false}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
    EXPECT_EQ(Core::ERROR_NONE, resetDone.Lock());

}

TEST_F(WarehouseInitializedTest, GenericResetDeviceNoResponse)
{
    Core::Event resetCallRxed(false, true);

EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke( 
            [&](const char *command, va_list args) {
                EXPECT_EQ(string(command), string("sh /lib/rdk/deviceReset.sh warehouse --suppressReboot &"));
                resetCallRxed.SetEvent();
                return Core::ERROR_NONE;
            }));

    //reset: suppress reboot: true - This doesn't generate any event (Expect no response)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("resetDevice"), _T("{\"suppressReboot\":true}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
    EXPECT_EQ(Core::ERROR_NONE, resetCallRxed.Lock());
}

TEST_F(WarehouseResetDeviceFailureTest, UserFactoryResetDeviceFailure)
{

EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](const char *command, va_list args) {
                EXPECT_EQ(string(command), string("sh /lib/rdk/deviceReset.sh warehouse --suppressReboot &"));
                return Core::ERROR_GENERAL;
            }));

    //reset: suppress reboot: true - This doesn't generate any event (Expect no response)
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("resetDevice"), _T("{\"suppressReboot\":true}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
    EXPECT_EQ(Core::ERROR_NONE, resetDone.Lock());
}

TEST_F(WarehouseInitializedTest, internalResetFailPassPhrase)
{
    //Invoke internalReset - No pass phrase
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("internalReset"), _T("{}"), response));

    //Invoke internalReset - Incorrect pass phrase
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("internalReset"), _T("{\"passPhrase\":\"Test Phrase\"}"), response));
}

TEST_F(WarehouseInitializedTest, internalResetScriptFail)
{
    EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](const char *command, va_list args) {
                return Core::ERROR_NONE;
            }));

    //Invoke internalReset - Correct pass phrase - Return error
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("internalReset"), _T("{\"passPhrase\":\"FOR TEST PURPOSES ONLY\"}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(WarehouseInitializedTest, internalReset)
{
    EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](const char *command, va_list args) {
                EXPECT_EQ(string(command), string("rm -rf /opt/drm /opt/www/whitebox /opt/www/authService && /rebootNow.sh -s WarehouseService &"));
                return Core::ERROR_NONE;
            }));

    //Invoke internalReset - Correct pass phrase - Return success
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("internalReset"), _T("{\"passPhrase\":\"FOR TEST PURPOSES ONLY\"}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(WarehouseInitializedTest, lightResetScriptFail)
{
    EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](const char *command, va_list args) {
                return Core::ERROR_NONE;
            }));

    //Invoke lightReset - returns error
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("lightReset"), _T("{}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(WarehouseInitializedTest, lightReset)
{
     EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
         .Times(1)
         .WillOnce(::testing::Invoke(
            [](const char *command, va_list args) {
                EXPECT_EQ(string(command), string("sh -c 'rm -rf /opt/netflix/* SD_CARD_MOUNT_PATH/netflix/* XDG_DATA_HOME/* XDG_CACHE_HOME/* XDG_CACHE_HOME/../.sparkStorage/ /opt/QT/home/data/* /opt/hn_service_settings.conf /opt/apps/common/proxies.conf /opt/lib/bluetooth /opt/persistent/rdkservicestore'"));
                return Core::ERROR_NONE;
            }));

    //Invoke lightReset
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("lightReset"), _T("{}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(WarehouseInitializedTest, isClean)
{
    const string userPrefFile = _T("/opt/user_preferences.conf");
    const uint8_t userPrefLang[] = "[General]\nui_language=US_en\n";
    const string customDataFile = _T("/lib/rdk/wh_api_5.conf");
    const uint8_t customDataFileContent[] = "[files]\n/opt/user_preferences.conf\n";

    //Invoke isClean - No conf file
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("isClean"), _T("{}"), response));
    EXPECT_EQ(response, _T("{\"success\":false,\"error\":\"Can't open file \\/lib\\/rdk\\/wh_api_5.conf\",\"clean\":false,\"files\":[]}"));

    //Invoke isClean - Empty conf file
    Core::File fileConf(customDataFile);
    Core::Directory(fileConf.PathName().c_str()).CreatePath();
    fileConf.Create();
    EXPECT_TRUE(Core::File(customDataFile).Exists());
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("isClean"), _T("{}"), response));
    EXPECT_EQ(response, _T("{\"error\":\"file \\/lib\\/rdk\\/wh_api_5.conf doesn't have any lines with paths\",\"success\":false,\"files\":[]}"));

    //Invoke isClean - Create empty conf file
    fileConf.Write(customDataFileContent, sizeof(customDataFileContent));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("isClean"), _T("{}"), response));
    EXPECT_EQ(response, _T("{\"success\":true,\"files\":[],\"clean\":true}"));

    //Invoke isClean - Add test data to conf file
    Core::File filePref(userPrefFile);
    filePref.Create();
    filePref.Write(userPrefLang, sizeof(userPrefLang));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("isClean"), _T("{}"), response));
    EXPECT_EQ(response, _T("{\"success\":true,\"files\":[\"\\/opt\\/user_preferences.conf\"],\"clean\":false}"));

    fileConf.Destroy();
    filePref.Destroy();
}

TEST_F(WarehouseInitializedTest, executeHardwareTest)
{
    EXPECT_CALL(*p_rfcApiImplMock, setRFCParameter(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke(
            [](char* pcCallerID, const char* pcParameterName, const char* pcParameterValue, DATA_TYPE eDataType) {
                EXPECT_EQ(string(pcCallerID), _T("Warehouse"));
                EXPECT_EQ(string(pcParameterName), _T("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.hwHealthTest.Enable"));
                EXPECT_EQ(string(pcParameterValue), _T("true"));
                EXPECT_EQ(eDataType, WDMP_BOOLEAN);
                return WDMP_SUCCESS;
            }))
        .WillOnce(::testing::Invoke(
            [](char* pcCallerID, const char* pcParameterName, const char* pcParameterValue, DATA_TYPE eDataType) {
                EXPECT_EQ(string(pcCallerID), _T("Warehouse"));
                EXPECT_EQ(string(pcParameterName), _T("Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.hwHealthTest.ExecuteTest"));
                EXPECT_EQ(string(pcParameterValue), _T("1"));
                EXPECT_EQ(eDataType, WDMP_INT);
                return WDMP_SUCCESS;
            }));

    //Invoke executeHardwareTest
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("executeHardwareTest"), _T("{}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(WarehouseInitializedTest, getHardwareTestResults)
{
    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                EXPECT_EQ(string(pcCallerID), string("Warehouse"));
                EXPECT_EQ(string(pcParameterName), string("Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.hwHealthTest.Results"));
                strncpy(pstParamData->value, "test", sizeof(pstParamData->value));
                return WDMP_SUCCESS;
            }));
    EXPECT_CALL(*p_rfcApiImplMock, setRFCParameter(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](char* pcCallerID, const char* pcParameterName, const char* pcParameterValue, DATA_TYPE eDataType) {
                EXPECT_EQ(string(pcCallerID), _T("Warehouse"));
                EXPECT_EQ(string(pcParameterName), _T("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.hwHealthTest.Enable"));
                EXPECT_EQ(string(pcParameterValue), _T("false"));
                EXPECT_EQ(eDataType, WDMP_BOOLEAN);
                return WDMP_SUCCESS;
            }));

    //Invoke getHardwareTestResults
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getHardwareTestResults"), _T("{}"), response));
    EXPECT_EQ(response, _T("{\"testResults\":\"test\",\"success\":true}"));
}

extern "C" FILE* __real_popen(const char* command, const char* type);
extern "C" int __real_pclose(FILE* pipe);
TEST_F(WarehouseResetDeviceTest, statusChangeEvent)
{
    IARM_BUS_PWRMgr_WareHouseOpn_EventData_t eventData = {IARM_BUS_PWRMGR_WAREHOUSE_RESET, IARM_BUS_PWRMGR_WAREHOUSE_COMPLETED};
    whMgrStatusChangeEventsHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_WAREHOUSEOPS_STATUSCHANGED, &eventData, 0);
    EXPECT_EQ(Core::ERROR_NONE, resetDone.Lock());
}
