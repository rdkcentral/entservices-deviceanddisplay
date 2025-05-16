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



#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mntent.h>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include <cstdio>
#include "UserSettingMock.h"
#include "ServiceMock.h"
#include "UserPreferences.h"
#include "ThunderPortability.h"
#include "COMLinkMock.h"
//#include "WrapsMock.h"

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

using ::testing::NiceMock;
using namespace WPEFramework;
using ::testing::Return;

namespace {
const string userPrefFile = _T("/opt/user_preferences.conf");
const uint8_t userPrefLang[] = "[General]\nui_language=US_en\n";
}

class UserPreferencesTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::UserPreferences> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    NiceMock<ServiceMock> service;
    Core::JSONRPC::Message message;
    NiceMock<COMLinkMock> comLinkMock;
    string response;
    NiceMock<UserSettingMock>* p_userSettingsMock = nullptr;  
    ServiceMock  *p_serviceMock  = nullptr;
    //NiceMock<WrapsImplMock>* p_wrapsImplMock = nullptr;
    std::string presentationLanguage;
    std::string currentPresentationLanguage = "en-US";

    UserPreferencesTest()
        : plugin(Core::ProxyType<Plugin::UserPreferences>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
    {
        p_userSettingsMock = new NiceMock<UserSettingMock>;
        // p_wrapsImplMock = new NiceMock<WrapsImplMock>;
        // Wraps::setImpl(p_wrapsImplMock);

        EXPECT_CALL(service, QueryInterfaceByCallsign(::testing::_, ::testing::_))
		    .WillRepeatedly(testing::Return(p_userSettingsMock));

        ON_CALL(*p_userSettingsMock, GetPresentationLanguage(::testing::_))
            .WillByDefault([this](std::string& language) {
                language = this->currentPresentationLanguage;
                return Core::ERROR_NONE;
            });

        ON_CALL(*p_userSettingsMock, SetPresentationLanguage(::testing::_))
            .WillByDefault([this](const std::string& language) {
                this->currentPresentationLanguage = language;
                return Core::ERROR_NONE;
            });

        ON_CALL(*p_userSettingsMock, GetMigrationState(::testing::_, ::testing::_))
            .WillByDefault([](const Exchange::IUserSettingsInspector::SettingsKey, bool& requiresMigration) {
                requiresMigration = false; // Assume migration not required
                return Core::ERROR_NONE;
            });


        ON_CALL(*p_userSettingsMock, Register(::testing::_))
            .WillByDefault(Return(Core::ERROR_NONE));


        // Initialize plugin with mock service
        plugin->Initialize(&service);
    }

    virtual ~UserPreferencesTest() {
        plugin->Deinitialize(&service);
        //Wraps::setImpl(nullptr);
        //delete p_wrapsImplMock;
        delete p_userSettingsMock;
    }
};

TEST_F(UserPreferencesTest, registeredMethods)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getUILanguage")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setUILanguage")));
}

TEST_F(UserPreferencesTest, paramsMissing)
{
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setUILanguage"), _T("{}"), response));
}

TEST_F(UserPreferencesTest, getUILanguage)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getUILanguage"), _T("{}"), response));
    EXPECT_EQ(response, _T("{\"ui_language\":\"US_en\",\"success\":true}"));
}

TEST_F(UserPreferencesTest, setUILanguage)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setUILanguage"), _T("{\"ui_language\":\"CA_fr\"}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getUILanguage"), _T("{}"), response));
    EXPECT_EQ(response, _T("{\"ui_language\":\"CA_fr\",\"success\":true}"));
}

