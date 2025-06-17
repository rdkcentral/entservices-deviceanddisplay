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
#include "L2Tests.h"
#include "L2TestsMock.h"
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <interfaces/IFrameRate.h>

#define JSON_TIMEOUT   (1000)
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);
#define FRAMERATE_CALLSIGN  _T("org.rdk.FrameRate.1")
#define FRAMERATEL2TEST_CALLSIGN _T("L2tests.1")

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using ::WPEFramework::Exchange::IFrameRate;

typedef enum : uint32_t {
    FrameRate_OnFpsEvent = 0x00000001,
    FrameRate_OnDisplayFrameRateChanging = 0x00000002, 
    FrameRate_OnDisplayFrameRateChanged = 0x00000003,
    FrameRate_StateInvalid = 0x00000000
}FrameRateL2test_async_events_t;

class AsyncHandlerMock_FrameRate
{
    public:
        AsyncHandlerMock_FrameRate()
        {
        }
        MOCK_METHOD(void, OnFpsEvent, (int average, int min, int max));
        MOCK_METHOD(void, OnDisplayFrameRateChanging, (const string& displayFrameRate));
        MOCK_METHOD(void, OnDisplayFrameRateChanged, (const string& displayFrameRate));
};

/* Notification Handler Class for COM-RPC*/
class FrameRateNotificationHandler : public Exchange::IFrameRate::INotification {
    private:
        /** @brief Mutex */
        std::mutex m_mutex;

        /** @brief Condition variable */
        std::condition_variable m_condition_variable;

        /** @brief Event signalled flag */
        uint32_t m_event_signalled;

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::IFrameRate::INotification)
        END_INTERFACE_MAP

    public:
        FrameRateNotificationHandler(){}
        ~FrameRateNotificationHandler(){}

    void OnFpsEvent(int average, int min, int max)
    {
        TEST_LOG("OnFpsEvent event triggered ***\n");
        std::unique_lock<std::mutex> lock(m_mutex);

        /* Notify the requester thread. */
        m_event_signalled |= FrameRate_OnFpsEvent;
        m_condition_variable.notify_one();
    }
    void OnDisplayFrameRateChanging(const string& displayFrameRate)
    {
        TEST_LOG("OnDisplayFrameRateChanging event triggered ***\n");
        std::unique_lock<std::mutex> lock(m_mutex);

        TEST_LOG("OnDisplayFrameRateChanging received: %s\n", displayFrameRate.c_str());
        /* Notify the requester thread. */
        m_event_signalled |= FrameRate_OnDisplayFrameRateChanging;
        m_condition_variable.notify_one();
    }
    void OnDisplayFrameRateChanged(const string& displayFrameRate)
    {
        TEST_LOG("OnDisplayFrameRateChanged event triggered ***\n");
        std::unique_lock<std::mutex> lock(m_mutex);

        TEST_LOG("OnDisplayFrameRateChanged received: %s\n", displayFrameRate.c_str());
        /* Notify the requester thread. */
        m_event_signalled |= FrameRate_OnDisplayFrameRateChanged;
        m_condition_variable.notify_one();
    }

    uint32_t WaitForRequestStatus(uint32_t timeout_ms,FrameRateL2test_async_events_t expected_status)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto now = std::chrono::system_clock::now();
        std::chrono::milliseconds timeout(timeout_ms);
        uint32_t signalled = FrameRate_StateInvalid;

        while (!(expected_status & m_event_signalled))
        {
            if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout)
            {
                TEST_LOG("Timeout waiting for request status event");
                break;
            }
        }

        signalled = m_event_signalled;
        return signalled;
    }
  };

/* FrameRate L2 test class declaration */
class FrameRate_L2test : public L2TestMocks {
protected:
    //IARM_EventHandler_t powerEventHandler = nullptr;

    FrameRate_L2test();
    virtual ~FrameRate_L2test() override;

    public:
        void OnFpsEvent(int average, int min, int max);
        void OnDisplayFrameRateChanging(const string& displayFrameRate);
        void OnDisplayFrameRateChanged(const string& displayFrameRate);

        /**
         * @brief waits for various status change on asynchronous calls
        */
        uint32_t WaitForRequestStatus(uint32_t timeout_ms,FrameRateL2test_async_events_t expected_status);
        uint32_t CreateFrameRateInterfaceObjectUsingComRPCConnection();

    private:
        /** @brief Mutex */
        std::mutex m_mutex;

        /** @brief Condition variable */
        std::condition_variable m_condition_variable;

        /** @brief Event signalled flag */
        uint32_t m_event_signalled;

    protected:
        /** @brief Pointer to the IShell interface */
        PluginHost::IShell *m_controller_framerate;

        /** @brief Pointer to the IFrameRate interface */
        Exchange::IFrameRate *m_framerateplugin;

        Core::Sink<FrameRateNotificationHandler> notify;
	
};

/**
 * @brief Constructor for FrameRate L2 test class
 */
FrameRate_L2test::FrameRate_L2test()
        : L2TestMocks()
{
    Core::hresult status = Core::ERROR_GENERAL;
    m_event_signalled = FrameRate_StateInvalid;

    /* Activate plugin in constructor */
    status = ActivateService("org.rdk.FrameRate");
    EXPECT_EQ(Core::ERROR_NONE, status);

    if (CreateFrameRateInterfaceObjectUsingComRPCConnection() != Core::ERROR_NONE) 
    {
        TEST_LOG("Invalid FrameRate_Client");
    } 
    else 
    {
        EXPECT_TRUE(m_controller_framerate != nullptr);
        if (m_controller_framerate) 
	{
            EXPECT_TRUE(m_framerateplugin != nullptr);
            if (m_framerateplugin) {
                m_framerateplugin->AddRef();
                m_framerateplugin->Register(&notify);
            } else {
                    TEST_LOG("m_framerateplugin is NULL");
            }
         } else {
             TEST_LOG("m_controller_framerate is NULL");
        }
    }
}

/**
 * @brief Destructor for FrameRate L2 test class
 */
FrameRate_L2test::~FrameRate_L2test()
{
    Core::hresult status = Core::ERROR_GENERAL;
    m_event_signalled = FrameRate_StateInvalid;

    if (m_framerateplugin) {
        m_framerateplugin->Unregister(&notify);
        m_framerateplugin->Release();
    }

    status = DeactivateService("org.rdk.FrameRate");
    EXPECT_EQ(Core::ERROR_NONE, status);
}

/**
 *
 * @param[in] message from FrameRate on the change
 */
void FrameRate_L2test::OnFpsEvent(int average, int min, int max)
{
    TEST_LOG("OnFpsEvent event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);
    /* Notify the requester thread. */
    m_event_signalled |= FrameRate_OnFpsEvent;
    m_condition_variable.notify_one();
}

void FrameRate_L2test::OnDisplayFrameRateChanging(const string& displayFrameRate)
{
    TEST_LOG("OnDisplayFrameRateChanging event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    TEST_LOG("OnDisplayFrameRateChanging received: %s\n", displayFrameRate.c_str());
    /* Notify the requester thread. */
    m_event_signalled |= FrameRate_OnDisplayFrameRateChanging;
    m_condition_variable.notify_one();
}
void FrameRate_L2test::OnDisplayFrameRateChanged(const string& displayFrameRate)
{
    TEST_LOG("OnDisplayFrameRateChanged event triggered ***\n");
    std::unique_lock<std::mutex> lock(m_mutex);

    TEST_LOG("OnDisplayFrameRateChanged received: %s\n", displayFrameRate.c_str());
    /* Notify the requester thread. */
    m_event_signalled |= FrameRate_OnDisplayFrameRateChanged;
    m_condition_variable.notify_one();
}
/**
 * @brief waits for various status change on asynchronous calls
 *
 * @param[in] timeout_ms timeout for waiting
 */
uint32_t FrameRate_L2test::WaitForRequestStatus(uint32_t timeout_ms,FrameRateL2test_async_events_t expected_status)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(timeout_ms);
    uint32_t signalled = FrameRate_StateInvalid;

    while (!(expected_status & m_event_signalled))
    {
        if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout)
        {
            TEST_LOG("Timeout waiting for request status event");
            break;
        }
    }

    signalled = m_event_signalled;

    return signalled;
}


/**
 * @brief Compare two request status objects
 *
 * @param[in] data Expected value
 * @return true if the argument and data match, false otherwise
 */
MATCHER_P(MatchRequestStatus, data, "")
{
    bool match = true;
    std::string expected;
    std::string actual;

    data.ToString(expected);
    arg.ToString(actual);
    TEST_LOG(" rec = %s, arg = %s",expected.c_str(),actual.c_str());
    EXPECT_STREQ(expected.c_str(),actual.c_str());

    return match;
}

//COM-RPC Changes
uint32_t FrameRate_L2test::CreateFrameRateInterfaceObjectUsingComRPCConnection()
{
    Core::hresult return_value =  Core::ERROR_GENERAL;
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> Engine_FrameRate;
    Core::ProxyType<RPC::CommunicatorClient> Client_FrameRate;

    TEST_LOG("Creating Engine_FrameRate");
    Engine_FrameRate = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    Client_FrameRate = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(Engine_FrameRate));

    TEST_LOG("Creating Engine_FrameRate Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    Engine_FrameRate->Announcements(mClient_FrameRate->Announcement());
#endif
    if (!Client_FrameRate.IsValid())
    {
        TEST_LOG("Invalid Client_FrameRate");
    }
    else
    {
        m_controller_framerate = Client_FrameRate->Open<PluginHost::IShell>(_T("org.rdk.FrameRate"), ~0, 3000);
        if (m_controller_framerate)
        {
        m_framerateplugin = m_controller_framerate->QueryInterface<Exchange::IFrameRate>();
        return_value = Core::ERROR_NONE;
        }
    }
    return return_value;
}

/************Test case Details **************************
** 1.LogApplicationEvent with eventName and eventValue for success case.
*******************************************************/

TEST_F(FrameRate_L2test, setCollectionFrequencyUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;

    int frequency=1000;
    bool success=false;
    status = m_framerateplugin->SetCollectionFrequency(frequency,success);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }  
}

/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case.
*******************************************************/

TEST_F(FrameRate_L2test, setCollectionFrequencyFailureUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;

    int frequency=0;
    bool success=false;
    status = m_framerateplugin->SetCollectionFrequency(frequency, success);
    EXPECT_EQ(status,Core::ERROR_INVALID_PARAMETER);
    if (status != Core::ERROR_INVALID_PARAMETER)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/************Test case Details **************************
** 1.LogApplicationEvent with eventName and eventValue for success using Jsonrpc.
*******************************************************/

TEST_F(FrameRate_L2test, setCollectionFrequencyUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With both Params expecting Success*/
    params["frequency"] = 1000;
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "setCollectionFrequency", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case using Jsonrpc.
*******************************************************/

TEST_F(FrameRate_L2test, setCollectionFrequencyFailurUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With one Param  expecting Fail case */
    params["frequency"] = 0;
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "setCollectionFrequency", params, result);
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

/************Test case Details **************************
** 1.LogApplicationEvent with eventName and eventValue for success case.
*******************************************************/

TEST_F(FrameRate_L2test, startFpsCollectionUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;

    bool success=false;
    uint32_t signalled = FrameRate_StateInvalid;

    status = m_framerateplugin->StartFpsCollection(success);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }  

    signalled = notify.WaitForRequestStatus(JSON_TIMEOUT,FrameRate_OnFpsEvent);
    EXPECT_TRUE(signalled & FrameRate_OnFpsEvent);
}

/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case.
*******************************************************/
#if 0
TEST_F(FrameRate_L2test, startFpsCollectionFailureUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;

    int frequency=0;
    bool success=false;
    status = m_framerateplugin->StartFpsCollection(success);
    EXPECT_EQ(status,Core::ERROR_INVALID_PARAMETER);
    if (status != Core::ERROR_INVALID_PARAMETER)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}
#endif
/************Test case Details **************************
** 1.LogApplicationEvent with eventName and eventValue for success using Jsonrpc.
*******************************************************/

TEST_F(FrameRate_L2test, startFpsCollectionUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With both Params expecting Success*/
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "startFpsCollection", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case using Jsonrpc.
*******************************************************/
#if 0
TEST_F(FrameRate_L2test, startFpsCollectionFailureUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With one Param  expecting Fail case */
    params["frequency"] = 0;
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "startFpsCollection", params, result);
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}
#endif

TEST_F(FrameRate_L2test, stopFpsCollectionUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;

    bool success=false;
    status = m_framerateplugin->StopFpsCollection(success);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }  
}

/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case.
*******************************************************/
#if 0
TEST_F(FrameRate_L2test, stopFpsCollectionFailureUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;

    int frequency=0;
    bool success=false;
    status = m_framerateplugin->StopFpsCollection(success);
    EXPECT_EQ(status,Core::ERROR_INVALID_PARAMETER);
    if (status != Core::ERROR_INVALID_PARAMETER)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}
#endif
/************Test case Details **************************
** 1.LogApplicationEvent with eventName and eventValue for success using Jsonrpc.
*******************************************************/

TEST_F(FrameRate_L2test, stopFpsCollectionUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With both Params expecting Success*/
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "stopFpsCollection", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}
/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case using Jsonrpc.
*******************************************************/
#if 0
TEST_F(FrameRate_L2test, stopFpsCollectionFailureUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With one Param  expecting Fail case */
    params["frequency"] = 0;
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "stopFpsCollection", params, result);
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}
#endif

/************Test case Details **************************
** 1.LogApplicationEvent with eventName and eventValue for success case.
*******************************************************/

TEST_F(FrameRate_L2test, updateFpsUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;

    int newfps = 30;
    bool success=false;
    status = m_framerateplugin->UpdateFps(newfps,success);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }  
}

/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case.
*******************************************************/

TEST_F(FrameRate_L2test, updateFpsFailureUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;

    int newfps=-1;
    bool success=false;
    status = m_framerateplugin->UpdateFps(newfps, success);
    EXPECT_EQ(status,Core::ERROR_INVALID_PARAMETER);
    if (status != Core::ERROR_INVALID_PARAMETER)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/************Test case Details **************************
** 1.LogApplicationEvent with eventName and eventValue for success using Jsonrpc.
*******************************************************/

TEST_F(FrameRate_L2test, updateFpsUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate>> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With both Params expecting Success*/
    params["newfps"] = 30;
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "updateFps", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case using Jsonrpc.
*******************************************************/

TEST_F(FrameRate_L2test, updateFpsFailureUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With one Param  expecting Fail case */
    params["newfps"] = -1;
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "updateFps", params, result);
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

/************Test case Details **************************
** 1.UploadReport with Upload status as SUCCESS.
*******************************************************/

TEST_F(FrameRate_L2test, setDisplayFrameRateUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;
    bool success = false;
    uint32_t signalled_pre = FrameRate_StateInvalid;
    uint32_t signalled_post = FrameRate_StateInvalid;

    status = m_framerateplugin->SetDisplayFrameRate("3840x2160px48",success);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
    
    signalled_pre = notify.WaitForRequestStatus(JSON_TIMEOUT,FrameRate_OnDisplayFrameRateChanging);
    EXPECT_TRUE(signalled_pre & FrameRate_OnDisplayFrameRateChanging);

    signalled_post = notify.WaitForRequestStatus(JSON_TIMEOUT,FrameRate_OnDisplayFrameRateChanged);
    EXPECT_TRUE(signalled_post & FrameRate_OnDisplayFrameRateChanged);
}

TEST_F(FrameRate_L2test, setDisplayFrameRateFailureUsingComrpc)
{
    Core::hresult status = Core::ERROR_INVALID_PARAMETER;
    bool success = false;
            
    status = m_framerateplugin->SetDisplayFrameRate("3840x2160p",success);
    EXPECT_EQ(status,Core::ERROR_INVALID_PARAMETER);
    if (status != Core::ERROR_INVALID_PARAMETER)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

TEST_F(FrameRate_L2test, setDisplayFrameRateUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate>> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With both Params expecting Success*/
    params["framerate"] = "3840x2160px48";
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "setDisplayFrameRate", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case using Jsonrpc.
*******************************************************/

TEST_F(FrameRate_L2test, setDisplayFrameRateFailureUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With one Param  expecting Fail case */
    params["framerate"] = "3840x2160p";
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "setDisplayFrameRate", params, result);
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

TEST_F(FrameRate_L2test, getDisplayFrameRateUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;
    bool success = false;
    std::string displayFrameRate;
            
    status = m_framerateplugin->GetDisplayFrameRate(displayFrameRate, success);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

TEST_F(FrameRate_L2test, getDisplayFrameRateUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate>> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With both Params expecting Success*/
    params["success"] = false;
    params["displayFrameRate"];
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "getDisplayFrameRate", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

TEST_F(FrameRate_L2test, setFrmModeUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;
    bool success = false;
    int frmmode = 0;
            
    status = m_framerateplugin->SetFrmMode(frmmode,success);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

TEST_F(FrameRate_L2test, setFrmModeFailureUsingComrpc)
{
    Core::hresult status = Core::ERROR_INVALID_PARAMETER;
    bool success = false;
    int frmmode = -1;
            
    status = m_framerateplugin->SetDisplayFrameRate(frmmode,success);
    EXPECT_EQ(status,Core::ERROR_INVALID_PARAMETER);
    if (status != Core::ERROR_INVALID_PARAMETER)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

TEST_F(FrameRate_L2test, setFrmModeUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate>> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With both Params expecting Success*/
    params["frmmode"] = 0;
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "setFrmMode", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case using Jsonrpc.
*******************************************************/

TEST_F(FrameRate_L2test, setFrmModeFailureUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With one Param  expecting Fail case */
    params["frmmode"] = -1;
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "setFrmMode", params, result);
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

TEST_F(FrameRate_L2test, getFrmModeUsingComrpc)
{
    Core::hresult status = Core::ERROR_GENERAL;
    bool success = false;
    int frmmode = 0;
            
    status = m_framerateplugin->GetFrmMode(frmmode,success);
    EXPECT_EQ(status,Core::ERROR_NONE);
    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

#if 0
TEST_F(FrameRate_L2test, getFrmModeFailureUsingComrpc)
{
    Core::hresult status = Core::ERROR_INVALID_PARAMETER;
    bool success = false;
    int frmmode = 1;
    std::string frmmode_str = std::to_string(frmmode);       
    status = m_framerateplugin->SetDisplayFrameRate(frmmode_str,success);
    EXPECT_EQ(status,Core::ERROR_INVALID_PARAMETER);
    if (status != Core::ERROR_INVALID_PARAMETER)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}
#endif 

TEST_F(FrameRate_L2test, getFrmModeUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate>> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With both Params expecting Success*/
    params["frmmode"] = 0;
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "getFrmMode", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

/************Test case Details **************************
** 1.LogApplicationEvent with empty eventName and eventValue for failure case using Jsonrpc.
*******************************************************/
#if 0
TEST_F(FrameRate_L2test, getFrmModeFailureUsingJsonrpc)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(FRAMERATE_CALLSIGN, FRAMERATEL2TEST_CALLSIGN);
    StrictMock<AsyncHandlerMock_FrameRate> async_handler;
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    JsonObject expected_status;

    /*With one Param  expecting Fail case */
    params["frmmode"] = 1;
    params["success"] = false;
    status = InvokeServiceMethod("org.rdk.FrameRate.1", "getFrmMode", params, result);
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, status);
    EXPECT_STREQ("null", result["value"].String().c_str());
}

#endif

/************Test case Details **************************
** 1.AbortReport with RBus Success case using Comrpc.
*******************************************************/


/************Test case Details **************************
** 1.setReportProfileStatus Without params and with Params as "No status".
** 2.setReportProfileStatus With Params as "STARTED".
** 3.setReportProfileStatus With Params as "COMPLETE".
** 4.setReportProfileStatus mocking RFC parameter and "NO Status" for failure case.
*******************************************************/
