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

#include "L2Tests.h"
#include "L2TestsMock.h"
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <interfaces/ISystemMode.h>
#include <interfaces/IDeviceOptimizeStateActivator.h>
#include <mutex>
#include <thread>

#define TEST_LOG(x, ...)                                                                                                                         \
    fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); \
    fflush(stderr);

#define SYSTEMMODE_CALLSIGN _T("org.rdk.SystemMode.1")
#define SYSTEMMODEL2TEST_CALLSIGN _T("L2tests.1")
#define DISPLAYSETTINGS_CALLSIGN _T("org.rdk.DisplaySettings")

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using ::WPEFramework::Exchange::ISystemMode;

class SystemMode_L2test : public L2TestMocks {
protected:
    virtual ~SystemMode_L2test() override;
    SystemMode_L2test();

public:
    uint32_t CreateSystemModeInterfaceObject();
    void SetUp() override;
    void TearDown() override;
    bool WaitForState(const ISystemMode::SystemMode systemMode,
        const ISystemMode::State expected,
        const std::chrono::milliseconds& timeout = std::chrono::milliseconds(3000),
        const std::chrono::milliseconds& interval = std::chrono::milliseconds(100));
    bool WaitForStateJsonRpc(const std::string& modeName,
        const std::string& expectedStateName,
        const std::chrono::milliseconds& timeout = std::chrono::milliseconds(3000),
        const std::chrono::milliseconds& interval = std::chrono::milliseconds(100));

private:
    /** @brief Mutex */
    std::mutex m_mutex;

    /** @brief Condition variable */
    std::condition_variable m_condition_variable;

    /** @brief Event signalled flag */
    uint32_t m_event_signalled;

protected:
    /** @brief Pointer to the IShell interface */
    PluginHost::IShell* m_controller_sysmode;

    /** @brief Pointer to the ISystemMode interface */
    Exchange::ISystemMode* m_sysmodeplugin;
};

SystemMode_L2test::SystemMode_L2test()
    : L2TestMocks()
{
    uint32_t status = Core::ERROR_GENERAL;

    /* Activate plugin in constructor */
    status = ActivateService("org.rdk.SystemMode");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

SystemMode_L2test::~SystemMode_L2test()
{
    uint32_t status = Core::ERROR_GENERAL;

    /* Deactivate plugin in destructor */
    status = DeactivateService("org.rdk.SystemMode");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

uint32_t SystemMode_L2test::CreateSystemModeInterfaceObject()
{
    uint32_t return_value = Core::ERROR_GENERAL;
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> SystemMode_Engine;
    Core::ProxyType<RPC::CommunicatorClient> SystemMode_Client;

    TEST_LOG("Creating SystemMode_Engine");
    SystemMode_Engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    SystemMode_Client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(SystemMode_Engine));

    TEST_LOG("Creating SystemMode_Engine Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    SystemMode_Engine->Announcements(mSystemMode_Client->Announcement());
#endif
    if (!SystemMode_Client.IsValid()) {
        TEST_LOG("Invalid SystemMode_Client");
    } else {
        m_controller_sysmode = SystemMode_Client->Open<PluginHost::IShell>(_T("org.rdk.SystemMode"), ~0, 3000);
        if (m_controller_sysmode) {
            m_sysmodeplugin = m_controller_sysmode->QueryInterface<Exchange::ISystemMode>();
            return_value = Core::ERROR_NONE;
        }
    }
    return return_value;
}

void SystemMode_L2test::SetUp()
{
    if ((m_sysmodeplugin == nullptr) || (m_controller_sysmode == nullptr)) {
        EXPECT_EQ(Core::ERROR_NONE, CreateSystemModeInterfaceObject());
    }
}

void SystemMode_L2test::TearDown()
{
    if (m_sysmodeplugin) {
        m_sysmodeplugin->Release();
        m_sysmodeplugin = nullptr;
    }
    if (m_controller_sysmode) {
        m_controller_sysmode->Release();
        m_controller_sysmode = nullptr;
    }
}

bool SystemMode_L2test::WaitForState(const ISystemMode::SystemMode systemMode,
    const ISystemMode::State expected,
    const std::chrono::milliseconds& timeout,
    const std::chrono::milliseconds& interval)
{
    auto start = std::chrono::steady_clock::now();
    Exchange::ISystemMode::GetStateResult result{};
    while (std::chrono::steady_clock::now() - start < timeout) {
        if (m_sysmodeplugin) {
            Core::hresult rc = m_sysmodeplugin->GetState(systemMode, result);
            if (rc == Core::ERROR_NONE && result.state == expected) {
                return true;
            }
        }
        std::this_thread::sleep_for(interval);
    }
    return false;
}

bool SystemMode_L2test::WaitForStateJsonRpc(const std::string& modeName,
    const std::string& expectedStateName,
    const std::chrono::milliseconds& timeout,
    const std::chrono::milliseconds& interval)
{
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < timeout) {
        JsonObject params;
        JsonObject result;
        params["systemMode"] = modeName;
        uint32_t rc = InvokeServiceMethod("org.rdk.SystemMode.1", "getState", params, result);
        if (rc == Core::ERROR_NONE) {
            if (result.HasLabel("state") && result["state"].String() == expectedStateName) {
                return true;
            }
        }
        std::this_thread::sleep_for(interval);
    }
    return false;
}

// Request VIDEO state and validate via polling using GetState
TEST_F(SystemMode_L2test, RequestState_VIDEO)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    Core::hresult rc = m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::VIDEO);
    EXPECT_EQ(Core::ERROR_NONE, rc);
    EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::VIDEO))
        << "VIDEO state not reached in time";
}

// Request GAME state and validate via polling
TEST_F(SystemMode_L2test, RequestState_GAME)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    Core::hresult rc = m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::GAME);
    EXPECT_EQ(Core::ERROR_NONE, rc);
    EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::GAME))
        << "GAME state not reached in time";
}

// Transition VIDEO -> GAME -> VIDEO validating each step
TEST_F(SystemMode_L2test, StateTransition_VIDEO_GAME_VIDEO)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    EXPECT_EQ(Core::ERROR_NONE, m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE, Exchange::ISystemMode::VIDEO));
    EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::VIDEO));
    EXPECT_EQ(Core::ERROR_NONE, m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE, Exchange::ISystemMode::GAME));
    EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::GAME));
    EXPECT_EQ(Core::ERROR_NONE, m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE, Exchange::ISystemMode::VIDEO));
    EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::VIDEO));
}

// Client activation / deactivation lifecycle
TEST_F(SystemMode_L2test, ClientActivationLifecycle)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    // The textual name of system mode as per @text: device_optimize
    const std::string modeName = "device_optimize";
    Core::hresult rcAct = m_sysmodeplugin->ClientActivated(SYSTEMMODEL2TEST_CALLSIGN, modeName);
    EXPECT_EQ(Core::ERROR_NONE, rcAct);
    Core::hresult rcDeact = m_sysmodeplugin->ClientDeactivated(SYSTEMMODEL2TEST_CALLSIGN, modeName);
    EXPECT_EQ(Core::ERROR_NONE, rcDeact);
}

// GetState after explicit request (ensures GetState returns latest value)
TEST_F(SystemMode_L2test, GetStateAfterRequest)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    EXPECT_EQ(Core::ERROR_NONE, m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE, Exchange::ISystemMode::VIDEO));
    ASSERT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::VIDEO));
    Exchange::ISystemMode::GetStateResult result{};
    EXPECT_EQ(Core::ERROR_NONE,
        m_sysmodeplugin->GetState(Exchange::ISystemMode::DEVICE_OPTIMIZE, result));
    EXPECT_EQ(Exchange::ISystemMode::VIDEO, result.state);
}

// ---------------- JSON-RPC TESTS ----------------

// JSON-RPC: requestState -> VIDEO then getState
TEST_F(SystemMode_L2test, JSONRPC_RequestState_VIDEO)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    JsonObject params;
    JsonObject result;
    uint32_t status = Core::ERROR_GENERAL;
    params["systemMode"] = "DEVICE_OPTIMIZE";
    params["state"] = "video";
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "video"));
}

// JSON-RPC: requestState -> GAME then getState
TEST_F(SystemMode_L2test, JSONRPC_RequestState_GAME)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    JsonObject params;
    JsonObject result;
    uint32_t status = Core::ERROR_GENERAL;
    params["systemMode"] = "DEVICE_OPTIMIZE";
    params["state"] = "game";
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "game"));
}

// JSON-RPC: getState after prior state request (ensure persistence)
TEST_F(SystemMode_L2test, JSONRPC_GetStateAfterRequest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    // First set to video
    {
        JsonObject params;
        JsonObject result;
        params["systemMode"] = "DEVICE_OPTIMIZE";
        params["state"] = "video";
        EXPECT_EQ(Core::ERROR_NONE, InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result));
        EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "video"));
    }
    // Now query
    JsonObject params;
    JsonObject result;
    params["systemMode"] = "DEVICE_OPTIMIZE";
    uint32_t status = InvokeServiceMethod("org.rdk.SystemMode.1", "getState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    ASSERT_TRUE(result.HasLabel("state"));
    EXPECT_EQ(result["state"].String(), "video");
}

// JSON-RPC: clientActivated then clientDeactivated
TEST_F(SystemMode_L2test, JSONRPC_ClientActivationLifecycle)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    params["callsign"] = SYSTEMMODEL2TEST_CALLSIGN;
    params["systemMode"] = "device_optimize";
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientActivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    // reuse params for deactivation
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientDeactivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
}

// JSON-RPC: Combined transition VIDEO -> GAME -> VIDEO
TEST_F(SystemMode_L2test, JSONRPC_StateTransition_VIDEO_GAME_VIDEO)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    JsonObject params;
    JsonObject result;
    uint32_t status;
    // VIDEO
    params["systemMode"] = "DEVICE_OPTIMIZE";
    params["state"] = "video";
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "video"));
    // GAME
    params["state"] = "game";
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "game"));
    // VIDEO
    params["state"] = "video";
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "video"));
}

// ClientActivated idempotency: activate twice, then deactivate once
TEST_F(SystemMode_L2test, Interface_ClientActivated_Idempotent)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    const std::string modeName = "DEVICE_OPTIMIZE";

    // First activation
    Core::hresult rcAct1 = m_sysmodeplugin->ClientActivated(DISPLAYSETTINGS_CALLSIGN, modeName);
    EXPECT_EQ(Core::ERROR_NONE, rcAct1);

    // Second activation should be handled gracefully (idempotent)
    Core::hresult rcAct2 = m_sysmodeplugin->ClientActivated(DISPLAYSETTINGS_CALLSIGN, modeName);
    EXPECT_EQ(Core::ERROR_NONE, rcAct2);

    // Cleanup: deactivate once
    Core::hresult rcDeact = m_sysmodeplugin->ClientDeactivated(DISPLAYSETTINGS_CALLSIGN, modeName);
    EXPECT_EQ(Core::ERROR_NONE, rcDeact);
}

// ClientDeactivated without prior activation should be handled gracefully
TEST_F(SystemMode_L2test, Interface_ClientDeactivated_WithoutActivation)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    const std::string modeName = "DEVICE_OPTIMIZE";

    // Attempt deactivation for a callsign that was not activated
    Core::hresult rcDeact = m_sysmodeplugin->ClientDeactivated("nonexistent.callsign", modeName);
    EXPECT_EQ(Core::ERROR_NONE, rcDeact);
}

// JSON-RPC: clientActivated idempotency (activate twice) then deactivate once
TEST_F(SystemMode_L2test, JSONRPC_ClientActivation_Idempotent)
{
    JsonObject params;
    JsonObject result;
    params["callsign"] = DISPLAYSETTINGS_CALLSIGN;
    params["systemMode"] = "DEVICE_OPTIMIZE";

    // First activation
    uint32_t status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientActivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);

    // Second activation (idempotency)
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientActivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);

    // Deactivate once
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientDeactivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
}