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
#include <interfaces/IDeviceDiagnostics.h>
#include <mutex>
#include <thread>

#define AV_POLL_TIMEOUT (31)
#define TEST_LOG(x, ...)                                                                                                                         \
    fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); \
    fflush(stderr);

#define DEVDIAG_CALLSIGN _T("org.rdk.DeviceDiagnostics.1")
#define DEVDIAGL2TEST_CALLSIGN _T("L2tests.1")

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using ::WPEFramework::Exchange::IDeviceDiagnostics;

typedef enum : uint32_t {
    DeviceDiagnostics_onAVDecoderStatusChanged = 0x00000001,
    DeviceDiagnostics_StateInvalid = 0x00000000
} DeviceDiagnosticsL2test_async_events_t;

class AsyncHandlerMock_DevDiag {
public:
    AsyncHandlerMock_DevDiag()
    {
    }
    MOCK_METHOD(void, onAVDecoderStatusChanged, (const JsonObject& message));
};

/* Notification Handler Class for COM-RPC*/
class DiagnosticsNotificationHandler : public Exchange::IDeviceDiagnostics::INotification {
private:
    /** @brief Mutex */
    std::mutex m_mutex;

    /** @brief Condition variable */
    std::condition_variable m_condition_variable;

    /** @brief Event signalled flag */
    uint32_t m_event_signalled;

    BEGIN_INTERFACE_MAP(Notification)
    INTERFACE_ENTRY(Exchange::IDeviceDiagnostics::INotification)
    END_INTERFACE_MAP

public:
    DiagnosticsNotificationHandler() {}
    ~DiagnosticsNotificationHandler() {}

    void OnAVDecoderStatusChanged(const string& message)
    {
        TEST_LOG("OnAVDecoderStatusChanged event triggered ***\n");
        std::unique_lock<std::mutex> lock(m_mutex);

        TEST_LOG("OnAVDecoderStatusChanged received: %s\n", message.c_str());
        /* Notify the requester thread. */
        m_event_signalled |= DeviceDiagnostics_onAVDecoderStatusChanged;
        m_condition_variable.notify_one();
    }

    uint32_t WaitForRequestStatus(uint32_t timeout_ms, DeviceDiagnosticsL2test_async_events_t expected_status)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto now = std::chrono::system_clock::now();
        std::chrono::seconds timeout(timeout_ms);
        uint32_t signalled = DeviceDiagnostics_StateInvalid;

        while (!(expected_status & m_event_signalled)) {
            if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
                TEST_LOG("Timeout waiting for request status event");
                break;
            }
        }
        signalled = m_event_signalled;
        return signalled;
    }
};

class DeviceDiagnostics_L2test : public L2TestMocks {
protected:
    virtual ~DeviceDiagnostics_L2test() override;
    DeviceDiagnostics_L2test();

public:
    void onAVDecoderStatusChanged(const JsonObject& message);

    uint32_t WaitForRequestStatus(uint32_t timeout_ms, DeviceDiagnosticsL2test_async_events_t expected_status);
    uint32_t CreateDeviceDiagnosticsInterfaceObject();

private:
    /** @brief Mutex */
    std::mutex m_mutex;

    /** @brief Condition variable */
    std::condition_variable m_condition_variable;

    /** @brief Event signalled flag */
    uint32_t m_event_signalled;

protected:
    /** @brief Pointer to the IShell interface */
    PluginHost::IShell* m_controller_devdiag;

    /** @brief Pointer to the IDeviceDiagnostics interface */
    Exchange::IDeviceDiagnostics* m_devdiagplugin;

    Core::Sink<DiagnosticsNotificationHandler> notify;
};

DeviceDiagnostics_L2test::DeviceDiagnostics_L2test()
    : L2TestMocks()
{
    uint32_t status = Core::ERROR_GENERAL;

    /* Activate plugin in constructor */
    status = ActivateService("org.rdk.DeviceDiagnostics");
    EXPECT_EQ(Core::ERROR_NONE, status);

    if (CreateDeviceDiagnosticsInterfaceObject() != Core::ERROR_NONE) {
        TEST_LOG("Invalid DeviceDiagnostics_Client");
    } else {
        EXPECT_TRUE(m_controller_devdiag != nullptr);
        if (m_controller_devdiag) {
            EXPECT_TRUE(m_devdiagplugin != nullptr);
            if (m_devdiagplugin) {
                m_devdiagplugin->AddRef();
                m_devdiagplugin->Register(&notify);
            } else {
                TEST_LOG("m_devdiagplugin is NULL");
            }
        } else {
            TEST_LOG("m_controller_devdiag is NULL");
        }
    }
}

DeviceDiagnostics_L2test::~DeviceDiagnostics_L2test()
{
    TEST_LOG("Inside DeviceDiagnostics_L2test destructor");

    if (m_devdiagplugin) {
        m_devdiagplugin->Unregister(&notify);
        m_devdiagplugin->Release();
    }

    uint32_t status = Core::ERROR_GENERAL;

    /* Deactivate plugin in destructor */
    status = DeactivateService("org.rdk.DeviceDiagnostics");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

void DeviceDiagnostics_L2test::onAVDecoderStatusChanged(const JsonObject& message)
{
    TEST_LOG("onAVDecoderStatusChanged event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    std::string str;
    message.ToString(str);

    TEST_LOG("onAVDecoderStatusChanged received: %s\n", str.c_str());

    /* Notify the requester thread. */
    m_event_signalled |= DeviceDiagnostics_onAVDecoderStatusChanged;
    m_condition_variable.notify_one();
}

uint32_t DeviceDiagnostics_L2test::WaitForRequestStatus(uint32_t timeout_ms, DeviceDiagnosticsL2test_async_events_t expected_status)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = std::chrono::system_clock::now();
    std::chrono::seconds timeout(timeout_ms);
    uint32_t signalled = DeviceDiagnostics_StateInvalid;

    while (!(expected_status & m_event_signalled)) {
        if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
            TEST_LOG("Timeout waiting for request status event");
            break;
        }
    }
    signalled = m_event_signalled;
    return signalled;
}

uint32_t DeviceDiagnostics_L2test::CreateDeviceDiagnosticsInterfaceObject()
{
    uint32_t return_value = Core::ERROR_GENERAL;
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> DeviceDiagnostics_Engine;
    Core::ProxyType<RPC::CommunicatorClient> DeviceDiagnostics_Client;

    TEST_LOG("Creating DeviceDiagnostics_Engine");
    DeviceDiagnostics_Engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    DeviceDiagnostics_Client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(DeviceDiagnostics_Engine));

    TEST_LOG("Creating DeviceDiagnostics_Engine Announcements");
    if (!DeviceDiagnostics_Client.IsValid()) {
        TEST_LOG("Invalid DeviceDiagnostics_Client");
    } else {
        m_controller_devdiag = DeviceDiagnostics_Client->Open<PluginHost::IShell>(_T("org.rdk.DeviceDiagnostics"), ~0, 3000);
        if (m_controller_devdiag) {
            m_devdiagplugin = m_controller_devdiag->QueryInterface<Exchange::IDeviceDiagnostics>();
            return_value = Core::ERROR_NONE;
        }
    }
    return return_value;
}

MATCHER_P(MatchRequestStatus, data, "")
{
    bool match = true;
    std::string expected;
    std::string actual;

    data.ToString(expected);
    arg.ToString(actual);
    TEST_LOG(" rec = %s, arg = %s", expected.c_str(), actual.c_str());
    EXPECT_STREQ(expected.c_str(), actual.c_str());

    return match;
}

/************Test case Details **************************
** 1.LogMilestone with no marker string.
** 2.LogMilestone with test marker string.
** 3.Validatethe marker was inserted in rdk_milestone.log file.
** All above cases using Jsonrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, LogMilestone_JSONRPC)
{
    // Create milestone logs file with dummy data
    std::ofstream milestoneFile("/opt/logs/rdk_milestones.log");
    if (milestoneFile.is_open()) {
        milestoneFile << "2024-01-10 10:15:23 [INFO] System boot initiated\n";
        milestoneFile << "2024-01-10 10:15:25 [INFO] Core services started\n";
        milestoneFile << "2024-01-10 10:15:28 [INFO] Network connectivity established\n";
        milestoneFile << "2024-01-10 10:15:30 [INFO] Device registration complete\n";
        milestoneFile << "2024-01-10 10:15:35 [INFO] Application framework initialized\n";
        milestoneFile << "2024-01-10 10:15:40 [INFO] System ready\n";
        milestoneFile.close();
        std::ifstream verifyFile("/opt/logs/rdk_milestones.log");
        EXPECT_TRUE(verifyFile.good());
        verifyFile.close();
    }

    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVDIAG_CALLSIGN, DEVDIAGL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_GENERAL;

    // testing with no marker string
    JsonObject params, result;
    params["marker"] = "";
    status = InvokeServiceMethod("org.rdk.DeviceDiagnostics.1", "logMilestone", params, result);
    EXPECT_EQ(Core::ERROR_GENERAL, status);

    // mocking logmilestone call
    RdkLoggerMilestone::getInstance().impl = p_rdkloggerImplMock;
    ON_CALL(*p_rdkloggerImplMock, logMilestone(::testing::_))
        .WillByDefault([](const char* tag) {
            TEST_LOG("LogMilestone called with tag: %s", tag);
            std::ofstream milestoneFile("/opt/logs/rdk_milestones.log", std::ios_base::app); // append in the existing file
            if (milestoneFile.is_open()) {
                milestoneFile << tag << "\n";
                milestoneFile.close();
            }
        });

    // testing with test marker string
    params["marker"] = "2024-01-10 10:15:40 [INFO] Test Marker.";
    status = InvokeServiceMethod("org.rdk.DeviceDiagnostics", "logMilestone", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);

    // check if the marker was inserted in the milestone file
    std::ifstream verifyFile("/opt/logs/rdk_milestones.log");
    ASSERT_TRUE(verifyFile.is_open()) << "Failed to open milestone log.";
    std::string line;
    std::string lastLine;
    while (std::getline(verifyFile, line)) {
        if (!line.empty()) {
            lastLine = line;
        }
    }
    verifyFile.close();
    EXPECT_EQ(lastLine, params["marker"].String().c_str());

    delete p_rdkloggerImplMock;
    RdkLoggerMilestone::getInstance().impl = nullptr;
}

/************Test case Details **************************
** 1.GetAVDecoderStatus with IDLE status using Jsonrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, IDLE_GetAVDecoderStatus_JSONRPC)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVDIAG_CALLSIGN, DEVDIAGL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_GENERAL;

    JsonObject params, result;
    params["avDecoderStatus"] = "";
    status = InvokeServiceMethod("org.rdk.DeviceDiagnostics.1", "getAVDecoderStatus", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_EQ(result["avDecoderStatus"].String(), "IDLE");
    TEST_LOG("GetAVDecoderStatus returned: %s", result["avDecoderStatus"].String().c_str());
}

/************Test case Details **************************
** 1.Register for onAVDecoderStatusChanged event change.
** 2.Mocking state change from IDLE to ACTIVE.
** 3.GetAVDecoderStatus with ACTIVE status using Jsonrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, ACTIVE_GetAVDecoderStatus_JSONRPC)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVDIAG_CALLSIGN, DEVDIAGL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_DevDiag> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    std::string message;
    uint32_t signalled = DeviceDiagnostics_StateInvalid;
    JsonObject expected_status;

    /* Register for onAVDecoderStatusChanged event. */
    status = jsonrpc.Subscribe<JsonObject>(AV_POLL_TIMEOUT,
        _T("onAVDecoderStatusChanged"),
        &AsyncHandlerMock_DevDiag::onAVDecoderStatusChanged,
        &async_handler);
    EXPECT_EQ(Core::ERROR_NONE, status);

    // Change the AVDecoderstatus from IDLE to ACTIVE
    EXPECT_CALL(*p_essRMgrMock, EssRMgrGetAVState(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::DoAll(
            ::testing::SetArgPointee<1>(EssRMgrRes_active),
            ::testing::Return(true)));

    message = "{\"avDecoderStatusChange\":\"{\\\"avDecoderStatusChange\\\":\\\"ACTIVE\\\"}\"}";
    expected_status.FromString(message);
    EXPECT_CALL(async_handler, onAVDecoderStatusChanged(MatchRequestStatus(expected_status)))
    .WillRepeatedly(Invoke(this, &DeviceDiagnostics_L2test::onAVDecoderStatusChanged));

    JsonObject param, result;
    param["avDecoderStatus"] = "";
    status = InvokeServiceMethod("org.rdk.DeviceDiagnostics.1", "getAVDecoderStatus", param, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_EQ(result["avDecoderStatus"].String(), "ACTIVE");

    signalled = WaitForRequestStatus(AV_POLL_TIMEOUT, DeviceDiagnostics_onAVDecoderStatusChanged);
    EXPECT_TRUE(signalled & DeviceDiagnostics_onAVDecoderStatusChanged);

    /*Unregister for event*/
    jsonrpc.Unsubscribe(AV_POLL_TIMEOUT, _T("onAVDecoderStatusChanged"));
}

/************Test case Details **************************
** 1.GetMilestones with success case using Jsonrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, GetMilestones_JSONRPC)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVDIAG_CALLSIGN, DEVDIAGL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_GENERAL;

    JsonObject params, result;
    status = InvokeServiceMethod("org.rdk.DeviceDiagnostics.1", "getMilestones", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(result["success"].Boolean());
}

/************Test case Details **************************
** 1.GetConfiguration with failure case using Jsonrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, GetConfiguration_JSONRPC)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(DEVDIAG_CALLSIGN, DEVDIAGL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    params["names"] = "Device.X_CISCO_COM_LED.RedPwm";

    JsonObject result;
    status = InvokeServiceMethod("org.rdk.DeviceDiagnostics.1", "getConfiguration", params, result);
    EXPECT_EQ(Core::ERROR_GENERAL, status);
    EXPECT_FALSE(result["success"].Boolean());
}

/************Test case Details **************************
** 1.LogMilestone with no marker string.
** 2.LogMilestone with test marker string.
** 3.Validatethe marker was inserted in rdk_milestone.log file.
** All above cases using Comrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, LogMilestone_COMRPC)
{
    uint32_t status = Core::ERROR_NONE;
    // Create milestone logs file with dummy data
    std::ofstream milestoneFile("/opt/logs/rdk_milestones.log");
    if (milestoneFile.is_open()) {
        milestoneFile << "2024-01-10 10:15:23 [INFO] System boot initiated\n";
        milestoneFile << "2024-01-10 10:15:25 [INFO] Core services started\n";
        milestoneFile << "2024-01-10 10:15:28 [INFO] Network connectivity established\n";
        milestoneFile << "2024-01-10 10:15:30 [INFO] Device registration complete\n";
        milestoneFile << "2024-01-10 10:15:35 [INFO] Application framework initialized\n";
        milestoneFile << "2024-01-10 10:15:40 [INFO] System ready\n";
        milestoneFile.close();
        std::ifstream verifyFile("/opt/logs/rdk_milestones.log");
        EXPECT_TRUE(verifyFile.good());
        verifyFile.close();
    }

    string marker;
    bool success = true;
    status = m_devdiagplugin->LogMilestone(marker, success);
    EXPECT_EQ(status, Core::ERROR_GENERAL);
    if (status != Core::ERROR_GENERAL) {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
    EXPECT_FALSE(success);

    RdkLoggerMilestone::getInstance().impl = p_rdkloggerImplMock;
    ON_CALL(*p_rdkloggerImplMock, logMilestone(::testing::_))
        .WillByDefault([](const char* tag) {
            TEST_LOG("LogMilestone called with tag: %s", tag);
            std::ofstream milestoneFile("/opt/logs/rdk_milestones.log", std::ios_base::app); // append in the existing file
            if (milestoneFile.is_open()) {
                milestoneFile << tag << "\n";
                milestoneFile.close();
            }
        });

    marker = "2024-01-10 10:15:40 [INFO] Test Marker.";
    status = m_devdiagplugin->LogMilestone(marker, success);
    EXPECT_TRUE(success);
    std::ifstream verifyFile("/opt/logs/rdk_milestones.log");
    ASSERT_TRUE(verifyFile.is_open()) << "Failed to open milestone log.";
    std::string line;
    std::string lastLine;
    while (std::getline(verifyFile, line)) {
        if (!line.empty()) {
            lastLine = line;
        }
    }
    verifyFile.close();

    EXPECT_EQ(lastLine, marker);
    EXPECT_EQ(status, Core::ERROR_NONE);
    if (status != Core::ERROR_NONE) {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
    delete p_rdkloggerImplMock;
    RdkLoggerMilestone::getInstance().impl = nullptr;
}

/************Test case Details **************************
** 1.GetAVDecoderStatus with IDLE status using Comrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, IDLE_GetAVDecoderStatus_COMRPC)
{
    uint32_t status = Core::ERROR_NONE;
    Exchange::IDeviceDiagnostics::AvDecoderStatusResult result;

    status = m_devdiagplugin->GetAVDecoderStatus(result);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(result.avDecoderStatus, "IDLE");
    TEST_LOG("GetAVDecoderStatus returned: %s", result.avDecoderStatus.c_str());
    if (status != Core::ERROR_NONE) {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/************Test case Details **************************
** 1.Mocking state change from IDLE to ACTIVE.
** 2.GetAVDecoderStatus with ACTIVE status using Comrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, ACTIVE_GetAVDecoderStatus_COMRPC)
{
    int32_t status = Core::ERROR_NONE;
    uint32_t signalled = DeviceDiagnostics_StateInvalid;

    // Change the AVDecoderstatus from IDLE to ACTIVE
    ON_CALL(*p_essRMgrMock, EssRMgrGetAVState(::testing::_, ::testing::_))
        .WillByDefault(::testing::DoAll(
            ::testing::SetArgPointee<1>(EssRMgrRes_active),
            ::testing::Return(true)));

    signalled = notify.WaitForRequestStatus(AV_POLL_TIMEOUT, DeviceDiagnostics_onAVDecoderStatusChanged);
    EXPECT_TRUE(signalled & DeviceDiagnostics_onAVDecoderStatusChanged);

    Exchange::IDeviceDiagnostics::AvDecoderStatusResult result;
    status = m_devdiagplugin->GetAVDecoderStatus(result);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(result.avDecoderStatus, "ACTIVE");
    TEST_LOG("GetAVDecoderStatus returned: %s", result.avDecoderStatus.c_str());
    if (status != Core::ERROR_NONE) {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
    EXPECT_EQ(result.avDecoderStatus, "ACTIVE");
}

/************Test case Details **************************
** 1.Mocking state change from ACTIVE to PAUSED.
** 2.GetAVDecoderStatus with PAUSED status using Comrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, PAUSED_GetAVDecoderStatus_COMRPC)
{
    int32_t status = Core::ERROR_NONE;
    uint32_t signalled = DeviceDiagnostics_StateInvalid;

    // Change the AVDecoderstatus from IDLE to PAUSED
    ON_CALL(*p_essRMgrMock, EssRMgrGetAVState(::testing::_, ::testing::_))
        .WillByDefault(::testing::DoAll(
            ::testing::SetArgPointee<1>(EssRMgrRes_paused),
            ::testing::Return(true)));

    signalled = notify.WaitForRequestStatus(AV_POLL_TIMEOUT, DeviceDiagnostics_onAVDecoderStatusChanged);
    EXPECT_TRUE(signalled & DeviceDiagnostics_onAVDecoderStatusChanged);

    Exchange::IDeviceDiagnostics::AvDecoderStatusResult result;
    status = m_devdiagplugin->GetAVDecoderStatus(result);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(result.avDecoderStatus, "PAUSED");
    TEST_LOG("GetAVDecoderStatus returned: %s", result.avDecoderStatus.c_str());
    if (status != Core::ERROR_NONE) {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
    EXPECT_EQ(result.avDecoderStatus, "PAUSED");
}

/************Test case Details **************************
** 1.GetConfiguration with error case when server not started.
** 2.validated output response using test server socket. 
** 3.GetConfiguration with success case using Comrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, GetConfiguration_COMRPC)
{
    uint32_t status = Core::ERROR_NONE;
    bool success = true;
    std::list<std::string> keys = {
        "Device.X_CISCO_COM_LED.RedPwm"
    };
    WPEFramework::RPC::IStringIterator* names;
    names = (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(keys));
    Exchange::IDeviceDiagnostics::IDeviceDiagnosticsParamListIterator* paramList = nullptr;
    // Error case server not started
    status = m_devdiagplugin->GetConfiguration(names, paramList,success);
    EXPECT_EQ(success,false);
    EXPECT_EQ(status, Core::ERROR_GENERAL);
    if (status != Core::ERROR_GENERAL) {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
    EXPECT_EQ(paramList, nullptr);

    // cleanup once the opertions are completed
    if (names) {
        names->Release();
        names = nullptr;
    }

    // server snippet
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_TRUE(sockfd != -1);

    //for port reuse
    int pt = 1;
    ASSERT_FALSE(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &pt, sizeof(pt)) < 0);

    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(10999);
    ASSERT_FALSE(bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0);
    ASSERT_FALSE(listen(sockfd, 10) < 0);

    std::thread thread = std::thread([&]() {
        auto addrlen = sizeof(sockaddr);
        const int connection = accept(sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
        ASSERT_FALSE(connection < 0);
        char buffer[2048] = { 0 };
        ASSERT_TRUE(read(connection, buffer, 2048) > 0);
        EXPECT_EQ(string(buffer), string(_T("POST / HTTP/1.1\r\nHost: 127.0.0.1:10999\r\nAccept: */*\r\nContent-Length: 98\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n{\"paramList\":[{\"name\":\"Device.X_CISCO_COM_LED.RedPwm\"},{\"name\":\"Device.DeviceInfo.Manufacturer\"}]}")));
        std::string response = _T("HTTP/1.1 200\n\rContent-type: application/json\n\r{\"paramList\":[{\"name\":\"Device.X_CISCO_COM_LED.RedPwm\",\"value\":\"123\"},{\"name\":\"Device.DeviceInfo.Manufacturer\",\"value\":\"RDK\"}],\"success\":true}");
        send(connection, response.c_str(), response.size(), 0);
        close(connection);
    });

    std::list<std::string> key = {
        "Device.X_CISCO_COM_LED.RedPwm",
        "Device.DeviceInfo.Manufacturer"
    };
    WPEFramework::RPC::IStringIterator* val;
    success = false;
    val = (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(key));
    status = m_devdiagplugin->GetConfiguration(val, paramList,success);
    EXPECT_EQ(success, true);

    EXPECT_EQ(status, Core::ERROR_NONE);
    if (status != Core::ERROR_NONE) {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }

    // Compare the expected output
    std::list<std::string> paramNames;
    if (paramList) {
        WPEFramework::Exchange::IDeviceDiagnostics::ParamList entry;
        paramList->Reset(0);

        while (paramList->Next(entry)) {
            paramNames.push_back(entry.name);
        }
    }
    EXPECT_EQ(key, paramNames);

    // cleanup the variables after the test is completed
    if (val) {
        val->Release();
        val = nullptr;
    }
    if (paramList) {
        paramList->Release();
        paramList = nullptr;
    }
    thread.join();
    close(sockfd);
}

/************Test case Details **************************
** 1.GetMilestones with success case using Comrpc.
** 2.GetMilestones with failure case by removing rdk_milestone log file using comrpc.
*******************************************************/

TEST_F(DeviceDiagnostics_L2test, GetMilestones_COMRPC)
{
    uint32_t status = Core::ERROR_NONE;
    bool success = false;
    WPEFramework::RPC::IStringIterator* result = nullptr;

    status = m_devdiagplugin->GetMilestones(result,success);
    EXPECT_EQ(status, Core::ERROR_NONE);
    if (status != Core::ERROR_NONE) {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
    EXPECT_EQ(success, true);
    if (result != nullptr) {
        string milestone;
        while (result->Next(milestone) == true) {
            // print the milestone logs
            TEST_LOG("Milestone: %s", milestone.c_str());
        }
        result->Release();
    }

    // remove the file
    remove("/opt/logs/rdk_milestones.log");

    // Expected file not found
    status = m_devdiagplugin->GetMilestones(result,success);
    EXPECT_EQ(status, Core::ERROR_GENERAL);
    if (status != Core::ERROR_GENERAL) {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
    EXPECT_EQ(success, false);
    EXPECT_EQ(result, nullptr);
}
