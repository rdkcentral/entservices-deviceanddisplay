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


#include "DisplayInfoMock.h"
#include <interfaces/IDisplayInfo.h>
#include "DisplayInfo.h"

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

class DisplayInfoTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::DisplayInfo> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    string response;

    DisplayInfoTest()
        : plugin(Core::ProxyType<Plugin::DisplayInfo>::Create())
        , handler(*plugin)
        , INIT_CONX(1, 0)
    {
    }
    virtual ~DisplayInfoTest() = default;
};

class DisplayInfoInitializedTest : public DisplayInfoTest {
protected:
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;
    ManagerImplMock   *p_managerImplMock = nullptr ;
    NiceMock<ServiceMock> service;
    Core::Sink<NiceMock<SystemInfo>> subSystem;

    DisplayInfoInitializedTest()
        : DisplayInfoTest()
    {
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        p_managerImplMock  = new NiceMock <ManagerImplMock>;
        device::Manager::setImpl(p_managerImplMock);

        ON_CALL(service, ConfigLine())
            .WillByDefault(::testing::Return("{\"root\":{\"mode\":\"Off\"}}"));
        ON_CALL(service, WebPrefix())
            .WillByDefault(::testing::Return(webPrefix));
        ON_CALL(service, SubSystems())
            .WillByDefault(::testing::Invoke(
                [&]() {
                    PluginHost::ISubSystem* result = (&subSystem);
                    result->AddRef();
                    return result;
                }));

        EXPECT_EQ(string(""), plugin->Initialize(&service));
    }
    virtual ~DisplayInfoInitializedTest() override
    {
        plugin->Deinitialize(&service);

        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr)
        {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }
        device::Manager::setImpl(nullptr);
        if (p_managerImplMock != nullptr)
        {
            delete p_managerImplMock;
            p_managerImplMock = nullptr;
        }
    }
};

class DisplayInfoInitializedDsTest : public DisplayInfoInitializedTest {
protected:
        HostImplMock             *p_hostImplMock = nullptr ;
        AudioOutputPortMock      *p_audioOutputPortMock = nullptr ;
        VideoResolutionMock      *p_videoResolutionMock = nullptr ;
        VideoOutputPortMock      *p_videoOutputPortMock = nullptr ;

    DisplayInfoInitializedDsTest()
        : DisplayInfoInitializedTest()
    {
        p_hostImplMock  = new NiceMock <HostImplMock>;
        device::Host::setImpl(p_hostImplMock);
        p_audioOutputPortMock  = new NiceMock <AudioOutputPortMock>;
        device::AudioOutputPort::setImpl(p_audioOutputPortMock);

        p_videoResolutionMock  = new NiceMock <VideoResolutionMock>;
        device::VideoResolution::setImpl(p_videoResolutionMock);
        p_videoOutputPortMock  = new NiceMock <VideoOutputPortMock>;
        device::VideoOutputPort::setImpl(p_videoOutputPortMock);
    }
    virtual ~DisplayInfoInitializedDsTest() override
    {
        device::AudioOutputPort::setImpl(nullptr);
        if (p_audioOutputPortMock != nullptr)
        {
            delete p_audioOutputPortMock;
            p_audioOutputPortMock = nullptr;
        }
        device::VideoResolution::setImpl(nullptr);
        if (p_videoResolutionMock != nullptr)
        {
            delete p_videoResolutionMock;
            p_videoResolutionMock = nullptr;
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
    }
};

class DisplayInfoInitializedDsVideoOutputTest : public DisplayInfoInitializedDsTest {
protected:
    VideoOutputPortConfigImplMock  *p_videoOutputPortConfigImplMock = nullptr ;
    VideoOutputPortTypeMock        *p_videoOutputPortTypeMock = nullptr ;

    DisplayInfoInitializedDsVideoOutputTest()
        : DisplayInfoInitializedDsTest()
    {
        p_videoOutputPortConfigImplMock  = new NiceMock <VideoOutputPortConfigImplMock>;
        device::VideoOutputPortConfig::setImpl(p_videoOutputPortConfigImplMock);
        p_videoOutputPortTypeMock  = new NiceMock <VideoOutputPortTypeMock>;
        device::VideoOutputPortType::setImpl(p_videoOutputPortTypeMock);
    }
    virtual ~DisplayInfoInitializedDsVideoOutputTest() override
    {
        device::VideoOutputPortType::setImpl(nullptr);
        if (p_videoOutputPortTypeMock != nullptr)
        {
            delete p_videoOutputPortTypeMock;
            p_videoOutputPortTypeMock = nullptr;
        }
        device::VideoOutputPortConfig::setImpl(nullptr);
        if (p_videoOutputPortConfigImplMock != nullptr)
        {
            delete p_videoOutputPortConfigImplMock;
            p_videoOutputPortConfigImplMock = nullptr;
        }
    }
};

TEST_F(DisplayInfoInitializedTest, registeredMethods)
{


}